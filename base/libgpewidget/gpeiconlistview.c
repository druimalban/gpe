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

#include "gpeiconlistview.h"
#include "gpeiconlistitem.h"

static guint my_signals[2];

struct _GPEIconListViewClass 
{
  GtkDrawingAreaClass parent_class;
  void (* clicked) (GPEIconListView * self, gpointer udata);
  void (* show_popup) (GPEIconListView * self, gpointer udata);
};

//private structure used to find an icon from its data, see _find_from_udata()
struct _Pack
{
  gpointer udata;         //udata to find
  GPEIconListItem *icon;  //the icon if found, NULL if not found
};

static GtkDrawingAreaClass *parent_class;

#define PARENT_HANDLER(___self,___allocation) \
	{ if(GTK_WIDGET_CLASS(parent_class)->size_allocate) \
		(* GTK_WIDGET_CLASS(parent_class)->size_allocate)(___self,___allocation); }


static gint _gpe_icon_list_view_popup (gpointer data);
static void _gpe_icon_list_view_cancel_popup (GPEIconListView *il);
static GPEIconListItem *_gpe_icon_list_view_get_icon (GPEIconListView *il, int col, int row);

#define LABEL_YMARGIN	8
#define TOP_MARGIN	5

#define il_label_height(_x)	(GPE_ICON_LIST_VIEW (_x)->label_height)
#define il_icon_size(_x)	(GPE_ICON_LIST_VIEW (_x)->icon_size)
#define il_row_height(_x)	(il_icon_size (_x) \
	                         + (((GPE_ICON_LIST_VIEW (_x))->flag_show_title) ? \
		                     (il_label_height (_x)) : 0 ) \
				 + LABEL_YMARGIN)
#define il_col_width(_x)	(il_icon_size (_x) + (2 * GPE_ICON_LIST_VIEW (_x)->icon_xmargin))


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

static void 
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

static gboolean
_gpe_icon_list_view_expose (GtkWidget *widget, GdkEventExpose *event)
{
  GPEIconListView *il;
  GList *icons;
  GdkPixbuf *dest;
  int row=0, col=0;
  PangoContext *pc;
  PangoLayout *pl;
  int label_height;
  int i;
	  
  il = GPE_ICON_LIST_VIEW (widget);
  
  /* Pango font rendering setup */
  if ((pc = gtk_widget_get_pango_context (widget)) == NULL)
    pc = gtk_widget_create_pango_context (widget);
  
  pl = pango_layout_new (pc);
  pango_layout_set_width (pl, il_col_width (il) * PANGO_SCALE);
  pango_layout_set_alignment (pl, PANGO_ALIGN_CENTER);
	  
  label_height = il_label_height (il);

  /* Make a new pixbuf for rendering icons to */
  dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
			 event->area.width, event->area.height);
  
  /* set the background color */
  gdk_pixbuf_fill (dest, il->bgcolor);
  
  /* Paint the background image, scaled */
  if (il->bgpixbuf)
    {
      /* ??? pb: what's this loop for?  why not just render the area of
	 background image given by event->area?  */
      for (i = 0; 
	   i < il->rows * il_row_height (il); 
	   i += gdk_pixbuf_get_height (il->bgpixbuf)) 
	{
	  GdkRectangle r1, dst;
	  r1.x = 0; r1.y = i;
	  r1.width = gdk_pixbuf_get_width (il->bgpixbuf);
	  r1.height = gdk_pixbuf_get_height (il->bgpixbuf);
	  
	  if (gdk_rectangle_intersect (&r1, &(event->area), &dst)) {
	    gdk_pixbuf_composite (il->bgpixbuf, dest,
				  dst.x-event->area.x,
				  dst.y-event->area.y,
				  dst.width, dst.height,
				  r1.x-event->area.x, r1.y-event->area.y,
				  1,
				  1,
				  GDK_INTERP_BILINEAR, 255);
	  }
	}
    }
  
  for (icons = il->icons; icons != NULL; icons = icons->next) 
    {
      GdkRectangle r1, r2, dst;
      GPEIconListItem *icon;
      GdkPixbuf *pixbuf=NULL;
      
      icon = icons->data;
      
      /* Compute & render the icon */
      r1.x=col * il_col_width (il);
      r1.y=row * il_row_height (il) + 5;
      r1.width = il_col_width (il);
      r1.height = il->icon_size;
      
      r2.x = event->area.x;
      r2.y = event->area.y;
      r2.width = event->area.width;
      r2.height = event->area.height;
	      
      if (gdk_rectangle_intersect (&r1, &r2, &dst)) 
	{
	  /* Get the icon from the cache if its there, if not put it there :) */
	  if (icon->pb_scaled)
	    pixbuf = icon->pb_scaled;
	  else 
	    {
	      if (icon->pb == NULL)
		icon->pb = gdk_pixbuf_new_from_file (icon->icon, NULL);
	      
	      _gpe_icon_list_view_check_icon_size (il, G_OBJECT (icon));
	      
	      if (icon->icon) 
		{
		  gdk_pixbuf_unref (icon->pb);
		  icon->pb = NULL;
		}
	      
	      pixbuf = icon->pb_scaled;
	    }
	  
	  if (pixbuf) 
	    {
	      // adjust X position for actual size
	      r1.width = gdk_pixbuf_get_width (pixbuf);
	      r1.x=col * il_col_width (il) + (il_col_width (il) - r1.width) / 2;
	      
	      // recompute intersection: maybe nothing to do any more
	      if (gdk_rectangle_intersect (&r1, &r2, &dst)) 
		{
		  gdk_pixbuf_composite (pixbuf, dest, // from, to
					dst.x - event->area.x, //dest_x
					dst.y - event->area.y,// + va->value, // dest_y
					dst.width, dst.height, // dest_width, dest_height
					r1.x - event->area.x, // offset_x
					r1.y - event->area.y, // + va->value, // offset_y
					1.0, 1.0,
					GDK_INTERP_BILINEAR, // filtering
					col == il->mcol && row == il->mrow ? 128 : 255);
		}
	    }
	}
      
      if (++col == il->cols) 
	{
	  col = 0; 
	  row++;
	}
    }
  
  /* Dump to drawingarea */
  gdk_pixbuf_render_to_drawable (dest, widget->window, widget->style->fg_gc[GTK_STATE_NORMAL],
				 0, 0, event->area.x, event->area.y,
				 event->area.width, event->area.height,
				 GDK_RGB_DITHER_NORMAL, event->area.x, event->area.y);
  
  gdk_pixbuf_unref (dest);
  
  col = row = 0;
  
  for (icons = il->icons; icons != NULL; icons = icons->next) 
    {
      GdkRectangle r1, r2, dst;
      GPEIconListItem *icon;
      icon = icons->data;
      
      if (il->flag_show_title && icon->title)
	{
	  /* if show_title mode is set and current title non NULL, then */
	  /* Compute & render the title */
	  r1.x=col * il_col_width (il);
	  r1.y=row * il_row_height (il) + il->icon_size + LABEL_YMARGIN;
	  r1.width = il_col_width (il);
	  r1.height = label_height;
	  
	  r2.x = event->area.x;
	  r2.y = event->area.y;
	  r2.width = event->area.width;
	  r2.height = event->area.height;
	  
	  if (gdk_rectangle_intersect (&r1, &r2, &dst)) 
	    {
	      char *stxt; int slen;
	      int selected=0;
	      PangoRectangle pr;
	      
	      stxt=icon->title;
	      slen = strlen (stxt);
	      
	      selected = (il->mrow == row && il->mcol == col);
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
	      
	      gtk_paint_layout (widget->style, widget->window, GTK_WIDGET_STATE (widget),
				FALSE, &dst,
				widget, "detail?wtf?", r1.x, r1.y, pl);
	    }
	} //(if show title, compute and render)
      
      if (++col == il->cols) 
	{
	  col = 0; 
	  row++;
	}
    }
  
  g_object_unref (pl);
  
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
  
  data = _gpe_icon_list_view_get_icon(il, il->mcol, il->mrow);
  if (data) 
    {
      gpe_icon_list_item_button_press (data, event);
      
      /* Register a popup if there is an icon under the cursor */
      il->popup_timeout = gtk_timeout_add (500, _gpe_icon_list_view_popup, (gpointer)il);
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
  if (col == il->mcol && row == il->mrow) {
    GPEIconListItem *data;
    data = _gpe_icon_list_view_get_icon(il, il->mcol, il->mrow);
    il->mcol = il->mrow = -1;
    if (data) {
      gpe_icon_list_item_button_release (data, event);
      g_signal_emit (G_OBJECT (widget), my_signals[0], 0, data->udata);
    }
  } else
    il->mcol = il->mrow = -1;
  
  return TRUE;
}

static void 
_gpe_icon_list_view_recalc_size (GPEIconListView *self, GtkAllocation *allocation)
{
  int count;
  int da_new_height;

  /* calculate number of columns that will fit.  If none will, use one anyway */
  self->cols = allocation->width / il_col_width (self);
  if (self->cols == 0)
    self->cols = 1;
	  
  count = g_list_length (self->icons);
		
  self->rows = (count - 1) / self->cols + 1;
  
  // update drawing area
  da_new_height = self->rows * il_row_height (self) + TOP_MARGIN;
  gtk_widget_set_usize (GTK_WIDGET (self), -1, da_new_height);
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
  GtkWidget *da = GTK_WIDGET (self);
  GObject *new = _gpe_icon_list_view_new_icon (title, icon, udata, NULL);
  self->icons = g_list_append (self->icons, new);
  _gpe_icon_list_view_recalc_size (self, &da->allocation);
  return new;
}

GObject *
gpe_icon_list_view_add_item_pixbuf (GPEIconListView *self, char *title, GdkPixbuf *icon, gpointer udata)
{
  GObject *new;
  GtkWidget *da = GTK_WIDGET (self);
  gdk_pixbuf_ref (icon);
  new = _gpe_icon_list_view_new_icon (title, NULL, udata, icon);
  self->icons = g_list_append (self->icons, new);
  _gpe_icon_list_view_check_icon_size (self, new);
  _gpe_icon_list_view_recalc_size (self, &da->allocation);
  return new;
}

void 
gpe_icon_list_view_remove_item (GPEIconListView *self, GObject *item)
{
  GtkWidget *da;
  da = GTK_WIDGET (self);
  self->icons = g_list_remove (self->icons, item);
  g_object_unref (item);
  _gpe_icon_list_view_recalc_size (self, &da->allocation);
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
  GtkWidget *da = GTK_WIDGET (self);
  for (iter = self->icons; iter; iter = iter->next) 
    {
      GPEIconListItem *i = GPE_ICON_LIST_ITEM (iter->data);
      g_object_unref (i);
    }
  
  g_list_free (self->icons);
  self->icons = NULL;
  
  _gpe_icon_list_view_recalc_size (self, &da->allocation);
}

void 
gpe_icon_list_view_set_icon_size (GPEIconListView *self, guint size)
{
  GList *iter;
  GtkWidget *da = GTK_WIDGET (self);
  
  for (iter = self->icons; iter; iter = iter->next) 
    {
      GPEIconListItem *i = GPE_ICON_LIST_ITEM (iter->data);
      if (i->pb_scaled)
	gdk_pixbuf_unref (i->pb_scaled);
      i->pb_scaled = NULL;
    }
  
  self->icon_size = size;
  
  _gpe_icon_list_view_recalc_size (self, &da->allocation);
  
  gtk_widget_draw (GTK_WIDGET (self), NULL);
}

static void
_gpe_icon_list_view_size_allocate (GtkWidget *self, GtkAllocation *allocation)
{
  GPEIconListView *il;
  il = GPE_ICON_LIST_VIEW (self);
  
  if ((int)g_object_get_data (G_OBJECT(il), "size_x") == allocation->width &&
      (int)g_object_get_data (G_OBJECT(il), "size_y") == allocation->height)
    return;
  
  g_object_set_data (G_OBJECT(il), "size_x", (gpointer)allocation->width);
  g_object_set_data (G_OBJECT(il), "size_y", (gpointer)allocation->height);
  
  PARENT_HANDLER (self, allocation);
  _gpe_icon_list_view_recalc_size (il, &self->allocation);	//luc: recalc *after* the size is allocated :)
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

static gint 
_gpe_icon_list_view_popup (gpointer data) 
{
  GPEIconListView *il = data;
  int row, col;
  
  row = il->mrow;
  col = il->mcol;
  
  g_signal_emit (G_OBJECT (il), my_signals[1], 0, _gpe_icon_list_view_get_icon(il,col, row)->udata);
  
  return FALSE;
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

static void
gpe_icon_list_view_init (GPEIconListView *self)
{
  self->rows = self->cols = 1;
  self->mrow = self->mcol = -1;
  self->icon_size = 48;
  self->icon_xmargin = 12;
  self->bgcolor = 0xffffffff;
  self->flag_embolden = TRUE;
  self->flag_show_title = TRUE;

  gtk_widget_add_events (GTK_WIDGET (self), GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  self->label_height = _gpe_icon_list_view_title_height (self);
}

static void
gpe_icon_list_view_class_init (GPEIconListViewClass * klass)
{
  GtkWidgetClass *widget_class;
  parent_class = g_type_class_ref (GTK_TYPE_DRAWING_AREA);

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

      type = g_type_register_static (GTK_TYPE_DRAWING_AREA, "GPEIconListView", &info, (GTypeFlags)0);
    }

  return type;
}

GtkWidget *
gpe_icon_list_view_new (void)
{
  return GTK_WIDGET (g_object_new (gpe_icon_list_view_get_type (), NULL));
}
