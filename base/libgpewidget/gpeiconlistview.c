/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>

#include "gpeiconlistview.h"
#include "gpeiconlistitem.h"

static guint my_signals[2];

struct _GPEIconListView
{
  GtkWidget class;
  
  /* private */
  GList *icons;
  GdkPixbuf * bgpixbuf;
  guint32 bgcolor;
  int rows;
  int cols;
  int mcol;
  int mrow;
  int popup_timeout;
  gboolean flag_embolden;
  gboolean flag_show_title;
  guint icon_size;
  guint icon_xmargin;
  guint label_height;

  int rows_set;
  t_gpe_textpos textpos;
};

struct _GPEIconListViewClass 
{
  GtkWidgetClass parent_class;
  void (* clicked) (GPEIconListView * self, gpointer udata);
  void (* show_popup) (GPEIconListView * self, gpointer udata);
};

//private structure used to find an icon from its data, see _find_from_udata()
struct _Pack
{
  gpointer udata;         //udata to find
  GPEIconListItem *icon;  //the icon if found, NULL if not found
};

static GtkWidgetClass *parent_class;

static void _gpe_icon_list_view_cancel_popup (GPEIconListView *il);
static GPEIconListItem *_gpe_icon_list_view_get_icon (GPEIconListView *il, int col, int row);

#define LABEL_YMARGIN	17
#define LABEL_XMARGIN   5
#define TOP_MARGIN	5

#define il_label_height(_x)	(GPE_ICON_LIST_VIEW (_x)->label_height)
#define il_icon_size(_x)	(GPE_ICON_LIST_VIEW (_x)->icon_size)
#define il_row_height(_x)	(il_icon_size (_x) \
	                         + ((((GPE_ICON_LIST_VIEW (_x))->flag_show_title) \
                            && ((GPE_ICON_LIST_VIEW (_x))->textpos == GPE_TEXT_BELOW))? \
		                     (il_label_height (_x)) : 0 ) \
				 + LABEL_YMARGIN)
#define il_col_width(_x)	(((GPE_ICON_LIST_VIEW (_x))->textpos == GPE_TEXT_BELOW) \
                    ? (il_icon_size (_x) \
                        + (2 * GPE_ICON_LIST_VIEW (_x)->icon_xmargin)) \
                   : (GTK_WIDGET (_x)->allocation.width))


/* Set the background */
void 
gpe_icon_list_view_set_bg (GPEIconListView *self, char *bg)
{
  if (self->bgpixbuf)
    gdk_pixbuf_unref (self->bgpixbuf);
  if (bg)
    self->bgpixbuf = gdk_pixbuf_new_from_file (bg, NULL);
  else
    self->bgpixbuf = NULL;
}

void 
gpe_icon_list_view_set_bg_pixmap (GPEIconListView *self, GdkPixbuf *bg)
{
  if (self->bgpixbuf)
    gdk_pixbuf_unref (self->bgpixbuf);
  gdk_pixbuf_ref (bg);
  self->bgpixbuf = bg;
}

void 
gpe_icon_list_view_set_bg_color (GPEIconListView *self, guint32 color)
{
  self->bgcolor = color;
}

/* Helper - max. height of a label */
static gint 
_gpe_icon_list_view_title_height (GPEIconListView *widget) 
{
  PangoContext *pc;
  PangoLayout *pl;
  PangoRectangle pr;
  int label_height;

  /* Pango font rendering setup */
  if ((pc = gtk_widget_get_pango_context (GTK_WIDGET(widget))) == NULL)
    pc = gtk_widget_create_pango_context (GTK_WIDGET(widget));
  pl = pango_layout_new (pc);
  
  /* Find out how tall icon labels will be */
  pango_layout_set_text (pl, "QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm\nQWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm", -1);
  pango_layout_get_pixel_extents  (pl, NULL, &pr);
  label_height = pr.height;
  
  g_object_unref (pl);
  
  return label_height;
}

static void 
_gpe_icon_list_view_get_rowcol (GPEIconListView *widget, int x, int y, int *col, int *row) 
{
  *col = x / il_col_width (widget);
  *row = y / il_row_height (widget);
}

static void 
_gpe_icon_list_view_refresh_containing (GPEIconListView *widget, int col, int row) 
{
  int x, y;
  
  x=col * il_col_width (widget);
  y=row * il_row_height (widget);
  
  gtk_widget_queue_draw_area (GTK_WIDGET (widget), x, y, il_col_width (widget), il_row_height (widget) + 5);
}

void 
_gpe_icon_list_view_check_icon_size (GPEIconListView *il, GObject *obj)
{
  GPEIconListItem *icon = GPE_ICON_LIST_ITEM (obj);

  if (icon->pb) 
    {
      guint h = gdk_pixbuf_get_height (icon->pb);
      if (h > il->icon_size + 1
	  || h < il->icon_size - 1) 
	{
	  double ratio = (double)il->icon_size / (double)h; 
	  guint new_width = gdk_pixbuf_get_width (icon->pb) * ratio;
	  icon->pb_scaled = gdk_pixbuf_scale_simple (icon->pb, new_width, il->icon_size, GDK_INTERP_BILINEAR);
	} 
      else 
	{
	  gdk_pixbuf_ref (icon->pb);
	  icon->pb_scaled = icon->pb;
	}
    }
}

void
_gpe_icon_list_view_queue_redraw (GPEIconListView *view, GPEIconListItem *i)
{
  GList *icons;
  int row = 0, col = 0;

  for (icons = view->icons; icons; icons = icons->next)
    {
      if (icons->data == i)
	break;
      col++;
      if (col == view->cols)
	{
	  col = 0;
	  row++;
	}
    }

  if (icons)
    {
      int x = il_col_width (view) * col;
      int y = il_row_height (view) * row;

      gtk_widget_queue_draw_area (GTK_WIDGET (view), x, y, il_col_width (view), il_row_height (view) + 5);
    }
}

static void
apply_translucency (GPEIconListView *il, GdkPixbuf *p)
{
  guchar *line;
  int i, j, w, h;
  int alpha, br, bg, bb;
  int stride, bpp;

  bb = il->bgcol & 0xff;
  bg = (il->bgcol >> 8) & 0xff;
  br = (il->bgcol >> 16) & 0xff;
  alpha = (il->bgcol >> 24) & 0xff;

  if (!p)
    return;
  
  line = gdk_pixbuf_get_pixels (p);
  w = gdk_pixbuf_get_width (p);
  h = gdk_pixbuf_get_height (p);
  bpp = gdk_pixbuf_get_bits_per_sample (p);
  stride = gdk_pixbuf_get_has_alpha (p) ? 4 : 3;

  for (j = 0; j < h; j++)
    {
      guchar *data = line;
      for (i = 0; i < w; i++)
	{
	  int r, g, b;
	  r = data[0];
	  g = data[1];
	  b = data[2];
	  r = ((r * (255 - alpha)) + (br * alpha)) / 256;
	  g = ((g * (255 - alpha)) + (bg * alpha)) / 256;
	  b = ((b * (255 - alpha)) + (bb * alpha)) / 256;
	  data[0] = r;
	  data[1] = g;
	  data[2] = b;
	  
	  data += stride;
	}
      line += gdk_pixbuf_get_rowstride (p);
    }
}

static gboolean
_gpe_icon_list_view_expose (GtkWidget *widget, GdkEventExpose *event)
{
  GPEIconListView *il;
  GList *icons;
  int row=0, col=0;
  PangoContext *pc;
  PangoLayout *pl;
  int label_height;
	  
  il = GPE_ICON_LIST_VIEW (widget);
  
  /* Pango font rendering setup */
  pc = gtk_widget_create_pango_context (widget);

  pl = pango_layout_new (pc);
  pango_layout_set_width (pl, il_col_width (il) * PANGO_SCALE);
  if (il->textpos == GPE_TEXT_BELOW)
    pango_layout_set_alignment (pl, PANGO_ALIGN_CENTER);
  else
    pango_layout_set_alignment (pl, PANGO_ALIGN_LEFT);
  
  label_height = il_label_height (il);

  if (il->bgpixbuf)
    {
      GdkPixbuf *s, *p;
      int ax, ay;
      ax = event->area.x + GTK_WIDGET (il)->allocation.x;
      ay = event->area.y + GTK_WIDGET (il)->allocation.y;
      
      s = gdk_pixbuf_new_subpixbuf (il->bgpixbuf,
				    ax, ay,
				    event->area.width, event->area.height);
      p = gdk_pixbuf_copy (s);
      apply_translucency (il, p);
      gdk_draw_pixbuf (widget->window, 
		       widget->style->fg_gc[GTK_STATE_NORMAL],
		       p,
		       0, 0,
		       event->area.x, event->area.y,
		       event->area.width, event->area.height,
		       GDK_RGB_DITHER_NORMAL, 0, 0);
      
      gdk_pixbuf_unref (s);
      gdk_pixbuf_unref (p);
    }

  for (icons = il->icons; icons != NULL; icons = icons->next) 
    {
      GdkRectangle r1, r2, dst;
      GPEIconListItem *icon;
      GdkPixbuf *pixbuf = NULL;
      int cell_x, cell_y, cell_w, cell_h;
 
      icon = icons->data;
      
      cell_x = col * il_col_width (il);
      cell_y = row * il_row_height (il) + 5;
      cell_w = il_col_width (il);
      cell_h = il->icon_size + LABEL_YMARGIN + label_height;

      r2.x = event->area.x;
      r2.y = event->area.y;
      r2.width = event->area.width;
      r2.height = event->area.height;

      r1.x = cell_x;
      r1.y = cell_y;
      r1.width = cell_w;
      r1.height = cell_h;
      
      if (gdk_rectangle_intersect (&r1, &r2, &dst))
	{
	  gboolean selected = FALSE;
	  GtkStateType state;

	  if (il->mrow == row && il->mcol == col)
	    selected = TRUE;

	  state = (selected && !il->flag_embolden) ? GTK_STATE_SELECTED : GTK_WIDGET_STATE (widget);

	  if (!il->bgpixbuf)
	    {
	      gtk_paint_flat_box (widget->style,
				  widget->window,
				  state,
				  GTK_SHADOW_NONE,
				  &dst,
				  widget,
				  "",
				  cell_x,
				  cell_y,
				  cell_w,
				  cell_h);
	    }
	  
	  /* Get the icon from the cache if its there, if not put it there :) */
	  if (icon->pb_scaled)
	    pixbuf = icon->pb_scaled;
	  else 
	    {
	      if (icon->pb == NULL && icon->icon != NULL)
		icon->pb = gdk_pixbuf_new_from_file (icon->icon, NULL);
	      
	      _gpe_icon_list_view_check_icon_size (il, G_OBJECT (icon));
	      
	      if (icon->icon && icon->pb)
		{
		  gdk_pixbuf_unref (icon->pb);
		  icon->pb = NULL;
		}
	      
	      pixbuf = icon->pb_scaled;
	    }
	  
	  if (pixbuf)
	    {
	      int pixbuf_w, pixbuf_h, x_offset;
	      
	      pixbuf_w = gdk_pixbuf_get_width (pixbuf);
	      pixbuf_h = gdk_pixbuf_get_height (pixbuf);
	      
	      // adjust X position for actual size
	      if (il->textpos == GPE_TEXT_BELOW)
	        x_offset = (il_col_width (il) - pixbuf_w) / 2;
	      else 
		x_offset = LABEL_XMARGIN;
          
	      r1.x = cell_x + x_offset;
	      r1.y = cell_y;
	      r1.width = pixbuf_w;
	      r1.height = pixbuf_h;
	      
	      if (gdk_rectangle_intersect (&r1, &r2, &dst))
		gdk_draw_pixbuf (widget->window, 
				 widget->style->fg_gc[widget->state],
				 pixbuf, 
				 dst.x - r1.x, dst.y - r1.y, 
				 dst.x, dst.y, 
				 dst.width, dst.height,
				 GDK_RGB_DITHER_NORMAL, 0, 0);
	    }
	  
	  if (il->flag_show_title && icon->title)
	    {
	      /* if show_title mode is set and current title non NULL, then */
	      /* Compute & render the title */
	      if (il->textpos == GPE_TEXT_BELOW)
		{
		  r1.x = cell_x;
		  r1.y = row * il_row_height (il) + il->icon_size + LABEL_YMARGIN - 8;
		  r1.width = cell_w;
		  r1.height = label_height;
		}
	      else
		{
		  r1.x = LABEL_XMARGIN + 2 * il->icon_size;
		  r1.y = row * il_row_height (il) + LABEL_YMARGIN - 8;
		  r1.width = widget->allocation.width - r1.x;
		  r1.height = label_height;
		}
	      
	      if (gdk_rectangle_intersect (&r1, &r2, &dst)) 
		{
		  char *stxt; int slen;
		  PangoRectangle pr;
		  
		  stxt=icon->title;
		  slen = strlen (stxt);
		  
		  if (selected && il->flag_embolden) 
		    {
		      char *newtxt;
		      newtxt = g_strdup_printf ("<b>%s</b>", stxt);
		      pango_layout_set_markup (pl, newtxt, -1);
		      g_free (newtxt);
		    } 
		  else
		    pango_layout_set_text (pl, stxt, -1);
		  
		  pango_layout_get_pixel_extents  (pl, NULL, &pr);
	      
		  while ((pango_layout_get_line_count (pl) > 2 ||
			  pr.width > il_col_width (il)) && slen>0) {
		    char *newtxt;
		    char *fmt;
		    if (stxt[slen-2] == ' ')
		      slen--;
		    
		    if (selected && il->flag_embolden)
		      fmt = g_strdup_printf ("<b>%%.%ds...</b>", --slen);
		    else
		      fmt = g_strdup_printf ("%%.%ds...", --slen);
		    
		    newtxt = g_strdup_printf (fmt, stxt);
		    g_free (fmt);
		    
		    if (selected)
		      pango_layout_set_markup (pl, newtxt, -1);
		    else
		      pango_layout_set_text (pl, newtxt, -1);
		    g_free (newtxt);
		    
		    pango_layout_get_pixel_extents  (pl, NULL, &pr);
		  }
		  
		  gtk_paint_layout (widget->style, widget->window, state,
				    FALSE, &dst, widget, "", r1.x, r1.y, pl);
		}
	    } //(if show title, compute and render)
	}

      if (++col == il->cols) 
	{
	  col = 0; 
	  row++;
	}
    }

  g_object_unref (pl);
  g_object_unref (pc);

  return TRUE;
}

static gint 
_gpe_icon_list_view_button_press (GtkWidget *widget, GdkEventButton *event)
{
  GPEIconListView *il;
  GPEIconListItem *data;
  
  il = GPE_ICON_LIST_VIEW (widget);

  _gpe_icon_list_view_get_rowcol (il, event->x, event->y, &(il->mcol), &(il->mrow));
  _gpe_icon_list_view_refresh_containing (il, il->mcol, il->mrow);
  
  data = _gpe_icon_list_view_get_icon (il, il->mcol, il->mrow);

  switch (event->button)
    {
    case 1:
      if (data)
	gpe_icon_list_item_button_press (data, event);
      break;

    case 3:
      g_signal_emit (G_OBJECT (il), my_signals[1], 0, data ? data->udata : NULL);
      break;
    }
  
  return TRUE;
}

static gint 
_gpe_icon_list_view_button_release (GtkWidget *widget, GdkEventButton *event) 
{
  GPEIconListView *il;
  int row, col;
  
  il = GPE_ICON_LIST_VIEW (widget);
  
  _gpe_icon_list_view_cancel_popup (il);
  
  _gpe_icon_list_view_refresh_containing (il, il->mcol, il->mrow);
  
  _gpe_icon_list_view_get_rowcol (il, event->x, event->y, &(col), &(row));
  if (col == il->mcol && row == il->mrow) 
    {
      GPEIconListItem *data;
      data = _gpe_icon_list_view_get_icon(il, il->mcol, il->mrow);
      il->mcol = il->mrow = -1;
      if (data) 
	{
	  gpe_icon_list_item_button_release (data, event);
	  g_signal_emit (G_OBJECT (widget), my_signals[0], 0, data->udata);
	}
    } 
  else
    il->mcol = il->mrow = -1;
  
  return TRUE;
}

static void 
_gpe_icon_list_view_recalc_size (GPEIconListView *self, 
				 GtkRequisition *req)
{
  int count;

  /* only one col in list mode, calculate otherwise */
  if (self->textpos == GPE_TEXT_RIGHT) 
    self->cols = 1;
  else
    {  
      /* calculate number of columns that will fit.  
	 If none will, use one anyway */
      self->cols = GTK_WIDGET (self)->allocation.width / il_col_width (self);
      if (self->cols == 0)
	self->cols = 1;
    }  
  count = g_list_length (self->icons);

  self->rows = (count - 1) / self->cols + 1;
  if (self->rows_set && self->rows > self->rows_set)
    self->rows = self->rows_set;

  if (count > self->cols)
    req->width = self->cols * il_col_width (self);
  else
    req->width = count * il_col_width (self);
  req->height = self->rows * il_row_height (self) + TOP_MARGIN;
}
	
static void
_gpe_icon_list_view_resize (GPEIconListView *self)
{
  _gpe_icon_list_view_recalc_size (self, &GTK_WIDGET (self)->requisition);
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static GObject *
_gpe_icon_list_view_new_icon (char *title, char *icon, gpointer udata, GdkPixbuf *pb)
{
  GPEIconListItem *ret;
  ret = (GPEIconListItem *) gpe_icon_list_item_new ();
  ret->title = title;
  ret->icon = icon;
  ret->udata = udata;
  ret->pb = pb;
  ret->pb_scaled = NULL;
  return G_OBJECT (ret);
}

GObject *
gpe_icon_list_view_add_item (GPEIconListView *self, char *title, char *icon, gpointer udata)
{
  GObject *new = _gpe_icon_list_view_new_icon (title, icon, udata, NULL);
  gpe_icon_list_item_set_parent (GPE_ICON_LIST_ITEM (new), self);
  self->icons = g_list_append (self->icons, new);
  if (GTK_WIDGET_REALIZED (self))
    _gpe_icon_list_view_resize (self);
  return new;
}

GObject *
gpe_icon_list_view_add_item_pixbuf (GPEIconListView *self, char *title, GdkPixbuf *icon, gpointer udata)
{
  GObject *new;
  gdk_pixbuf_ref (icon);
  new = _gpe_icon_list_view_new_icon (title, NULL, udata, icon);
  gpe_icon_list_item_set_parent (GPE_ICON_LIST_ITEM (new), self);
  self->icons = g_list_append (self->icons, new);
  _gpe_icon_list_view_check_icon_size (self, new);
  _gpe_icon_list_view_resize (self);
  return new;
}

void 
gpe_icon_list_view_remove_item (GPEIconListView *self, GObject *item)
{
  self->icons = g_list_remove (self->icons, item);
  g_object_unref (item);
  if (GTK_WIDGET_REALIZED (self))
    _gpe_icon_list_view_resize (self);
}

void 
gpe_icon_list_view_set_item_icon (GPEIconListView *self, GObject *item, GdkPixbuf *new_pixbuf)
{
  GPEIconListItem *i;
  
  i = GPE_ICON_LIST_ITEM (item);
  
  if (i->pb)
    gdk_pixbuf_unref (i->pb);
  if (i->pb_scaled)
    gdk_pixbuf_unref (i->pb_scaled);
  
  i->pb = new_pixbuf;
  gdk_pixbuf_ref (i->pb);
  
  _gpe_icon_list_view_check_icon_size (self, item);
}

void 
gpe_icon_list_view_set_embolden (GPEIconListView *self, gboolean yes)
{
  self->flag_embolden = yes;
}

void
gpe_icon_list_view_set_show_title (GPEIconListView *self, gboolean yes)
{
  self->flag_show_title = yes;
}

void 
gpe_icon_list_view_set_icon_xmargin (GPEIconListView *self, guint margin)
{
  self->icon_xmargin = margin;
}

void 
gpe_icon_list_view_clear (GPEIconListView *self)
{
  GList *iter;
  for (iter = self->icons; iter; iter = iter->next) 
    {
      GPEIconListItem *i = GPE_ICON_LIST_ITEM (iter->data);
      g_object_unref (i);
    }
  
  g_list_free (self->icons);
  self->icons = NULL;
  
  if (GTK_WIDGET_REALIZED (self))
    _gpe_icon_list_view_resize (self);
}

void 
gpe_icon_list_view_set_icon_size (GPEIconListView *self, guint size)
{
  GList *iter;
  
  for (iter = self->icons; iter; iter = iter->next) 
    {
      GPEIconListItem *i = GPE_ICON_LIST_ITEM (iter->data);
      if (i->pb_scaled)
	gdk_pixbuf_unref (i->pb_scaled);
      i->pb_scaled = NULL;
    }
  
  self->icon_size = size;
  
  if (GTK_WIDGET_REALIZED (self))
    _gpe_icon_list_view_resize (self);
  
  gtk_widget_draw (GTK_WIDGET (self), NULL);
}

static void
_gpe_icon_list_view_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
  _gpe_icon_list_view_recalc_size (GPE_ICON_LIST_VIEW (widget),
				   requisition);
}

static void
_gpe_icon_list_view_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  GPEIconListView *view;
  gboolean size_change = TRUE;

  view = GPE_ICON_LIST_VIEW (widget);

  if (allocation->width == widget->allocation.width)
    size_change = FALSE;
  
  widget->allocation = *allocation;

  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_move_resize (widget->window,
			    allocation->x, allocation->y,
			    allocation->width, allocation->height);
  
  //luc: recalc *after* the size is allocated :)
  if (size_change)
    _gpe_icon_list_view_resize (view);
}

static void
_gpe_icon_list_view_realize (GtkWidget *widget)
{
  GdkWindowAttr attributes;
  gint attributes_mask;

  _gpe_icon_list_view_resize (GPE_ICON_LIST_VIEW (widget));

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
    
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
			    GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, widget);

  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
}

static GPEIconListItem *
_gpe_icon_list_view_get_icon (GPEIconListView *il, int col, int row) 
{
  int i;
  GList *icons;
  
  icons = il->icons;
  for (i=0;i<row*il->cols+col && icons;i++)
    icons = icons->next;
  
  return icons ? icons->data : NULL;
}

static void 
_gpe_icon_list_view_cancel_popup (GPEIconListView *il) 
{
  if (il->popup_timeout != 0) {
    gtk_timeout_remove (il->popup_timeout);
    il->popup_timeout = 0;
  }
}

void 
gpe_icon_list_view_popup_removed (GPEIconListView *self) 
{
  int row, col;
  
  row = self->mrow;
  col = self->mcol;
  
  self->mcol = self->mrow = -1;
  
  _gpe_icon_list_view_refresh_containing (self, col, row);
}

void
gpe_icon_list_view_set_rows (GPEIconListView *self, guint rows)
{
  self->rows_set = rows;
}

void
gpe_icon_list_view_set_textpos (GPEIconListView *self, t_gpe_textpos textpos)
{
  if (self->textpos == textpos) 
      return;
  self->textpos = textpos;
  if (GTK_WIDGET_REALIZED (self))
    _gpe_icon_list_view_resize (self);
}

static void
gpe_icon_list_view_init (GPEIconListView *self)
{
  self->rows = self->cols = 1;
  self->mrow = self->mcol = -1;
  self->icon_size = 48;
  self->icon_xmargin = 12;
  self->bgcolor = 0xd0ffffff;
  self->flag_embolden = FALSE;
  self->flag_show_title = TRUE;
  self->rows_set = 0;
  self->textpos = GPE_TEXT_BELOW;

  self->label_height = _gpe_icon_list_view_title_height (self);
}

static void
gpe_icon_list_view_class_init (GPEIconListViewClass * klass)
{
  GtkWidgetClass *widget_class;
  parent_class = g_type_class_ref (GTK_TYPE_WIDGET);

  my_signals[0] = g_signal_new ("clicked",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (struct _GPEIconListViewClass, clicked),
				NULL, NULL,
				gtk_marshal_VOID__BOXED,
				G_TYPE_NONE, 1,
				G_TYPE_POINTER);

  my_signals[1] = g_signal_new ("show_popup",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (struct _GPEIconListViewClass, show_popup),
				NULL, NULL,
				gtk_marshal_VOID__BOXED,
				G_TYPE_NONE, 1,
				G_TYPE_POINTER);

  widget_class = (GtkWidgetClass *) klass;
  widget_class->expose_event = _gpe_icon_list_view_expose;
  widget_class->button_press_event = _gpe_icon_list_view_button_press;
  widget_class->button_release_event = _gpe_icon_list_view_button_release;
  widget_class->size_allocate = _gpe_icon_list_view_size_allocate;
  widget_class->size_request = _gpe_icon_list_view_size_request;
  widget_class->realize = _gpe_icon_list_view_realize;
}

static void
gpe_icon_list_view_fini (GPEIconListView *self)
{
}

GType
gpe_icon_list_view_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (GPEIconListViewClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) gpe_icon_list_view_fini,
	(GClassInitFunc) gpe_icon_list_view_class_init,
	(GClassFinalizeFunc) NULL,
	NULL /* class_data */,
	sizeof (GPEIconListView),
	0 /* n_preallocs */,
	(GInstanceInitFunc) gpe_icon_list_view_init,
      };

      type = g_type_register_static (GTK_TYPE_WIDGET, "GPEIconListView", &info, (GTypeFlags)0);
    }

  return type;
}

GtkWidget *
gpe_icon_list_view_new (void)
{
  return GTK_WIDGET (g_object_new (gpe_icon_list_view_get_type (), NULL));
}
