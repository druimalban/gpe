/*
 * gpe-appmgr - a program launcher
 *
 * Copyright (c) 2004 Phil Blundell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <string.h>
#include <unistd.h>
#include <sys/signal.h>

/* Gtk etc. */
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>

/* I/O */
#include <stdio.h>

/* malloc / free */
#include <stdlib.h>

#include <unistd.h>

/* time() */
#include <time.h>

/* i18n */
#include <libintl.h>
#include <locale.h>

/* GPE */
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>
#include <gpe/spacing.h>
#include <gpe/gpeiconlistview.h>
#include <gpe/launch.h>
#include <gpe/infoprint.h>
#include <gpe/errorbox.h>

/* everything else */
#include "main.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <cairo.h>
#include <math.h>

//#define DEBUG
#ifdef DEBUG
#define DBG(x) {fprintf x ;}
#define TRACE(x) {fprintf(stderr,"TRACE: " x "\n");}
#else
#define TRACE(x) ;
#define DBG(x) ;
#endif

GtkWidget *programs_fixed;
GdkPixbuf *background;
GtkWidget *toplevel;
GdkPixbuf *shutdown_pixbuf, *shutdown_down_pixbuf;
gboolean shutdown_is_down;
GtkWidget *tab_notebook;

static GSList *views;
static gchar *background_filename;

static GSList *rows, *labels;

#define DEFAULT_BACKGROUND PREFIX "/share/gpe/pixmaps/default/gpe-appmgr/desktop-background.png"

#define ICONLIST_WIDTH		560
#define ICONLIST_XOFFSET	52

#define LABEL_WIDTH		24

#define N_(x)  (x)

GdkGC *bg_gc;

void
row_view_set_bg_colour (guint c)
{
  GSList *v;

  for (v = views; v; v = v->next)
    {
      gpe_icon_list_view_set_bg_color (GPE_ICON_LIST_VIEW (v->data), c);
      gtk_widget_queue_draw (GTK_WIDGET (v->data));
    }
}

void
row_view_refresh_background (void)
{
  GSList *ll;

  for (ll = rows; ll; ll = ll->next)
    gpe_icon_list_view_set_bg_pixmap (GPE_ICON_LIST_VIEW (ll->data), background);

  if (toplevel)
    gtk_widget_queue_draw (toplevel);
}

void
row_view_set_background (gchar *path)
{
  GdkPixbuf *pix;
  int width, height;

  if (background)
    g_object_unref (background);
  if (background_filename)
    g_free (background_filename);

  if (path == NULL || path[0] == 0)
    path = DEFAULT_BACKGROUND;

  background_filename = g_strdup (path);
  pix = gdk_pixbuf_new_from_file (background_filename, NULL);
  if (!pix)
    pix = gdk_pixbuf_new_from_file (DEFAULT_BACKGROUND, NULL);
  gdk_drawable_get_size (toplevel->window, &width, &height);
  background = gdk_pixbuf_scale_simple (pix, width, height, GDK_INTERP_BILINEAR);

  row_view_refresh_background ();
}

static void
run_callback (GObject *obj, GdkEventButton *ev, GnomeDesktopFile *p)
{
  run_package (p, obj);
}

static GtkWidget *
make_label (char *text, int height)
{
  GdkVisual *gv;
  GdkColormap *gcm;
  GdkPixmap *pixmap;
  cairo_t *cr;
  cairo_surface_t *surface;
  cairo_text_extents_t extents;

  pixmap = gdk_pixmap_new (programs_fixed->window, LABEL_WIDTH, height, -1);

  gv = gdk_window_get_visual (programs_fixed->window);
  gcm = gdk_drawable_get_colormap (programs_fixed->window);

  cr = cairo_create ();
  surface = cairo_xlib_surface_create (GDK_WINDOW_XDISPLAY (pixmap), GDK_WINDOW_XWINDOW (pixmap), 
				       gdk_x11_visual_get_xvisual (gv), 0, 
				       gdk_x11_colormap_get_xcolormap (gcm));

  cairo_set_target_surface (cr, surface);

  cairo_set_rgb_color (cr, 1.0, 1.0, 1.0);
  cairo_rectangle (cr, 0, 0, LABEL_WIDTH, height);
  cairo_fill (cr);

  cairo_set_rgb_color (cr, 0xa5 / 255.0, 0xae / 255.0, 0xbf / 255.0);
  cairo_move_to (cr, LABEL_WIDTH, 0);
  cairo_line_to (cr, 0, 0);
  cairo_line_to (cr, 0, height);
  cairo_line_to (cr, LABEL_WIDTH, height);
  cairo_stroke (cr);

  cairo_select_font (cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
		     CAIRO_FONT_WEIGHT_BOLD);
  cairo_scale_font (cr, 16.0);

  cairo_text_extents (cr, text, &extents);

  cairo_move_to (cr, 18, (height + extents.width) / 2);

  cairo_rotate (cr, -M_PI/2);
  cairo_set_rgb_color (cr, 0, 0, 1.0);
  cairo_show_text (cr, text);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);

  return gtk_image_new_from_pixmap (pixmap, NULL);
}

/* Make the contents for a notebook tab.
 * Generally we only want one group, but NULL means
 * ignore group setting (ie. "All").
 */
static GtkWidget *
create_row (GList *all_items, char *current_group)
{
  GtkWidget *view;
  GList *this_item;

  view = gpe_icon_list_view_new ();

  gpe_icon_list_view_set_icon_size (GPE_ICON_LIST_VIEW (view), 32);
  gpe_icon_list_view_set_icon_xmargin (GPE_ICON_LIST_VIEW (view), 24);

  gpe_icon_list_view_set_bg_pixmap (GPE_ICON_LIST_VIEW (view), background);
  gpe_icon_list_view_set_bg_color (GPE_ICON_LIST_VIEW (view), 0x00ff8080);
  gpe_icon_list_view_set_border_color (GPE_ICON_LIST_VIEW (view), 0x00bfaea5);
  gpe_icon_list_view_set_border_width (GPE_ICON_LIST_VIEW (view), 8);

  views = g_slist_append (views, view);
  
  this_item = all_items;
  while (this_item)
    {
      GnomeDesktopFile *p;
      GObject *item;
      gchar *name = NULL;
 
      p = (GnomeDesktopFile *) this_item->data;
     
      gnome_desktop_file_get_string (p, NULL, "Name", &name);
 
      item = gpe_icon_list_view_add_item (GPE_ICON_LIST_VIEW (view),
					  name,
					  get_icon_fn (p, 48),
					  (gpointer)p);

      g_signal_connect (item, "button-release", G_CALLBACK (run_callback), p);
      
      this_item = this_item->next;
    }
  
  gtk_widget_show (view);
  return view;
}

static int
add_row (GtkWidget *row, gint cur_y, gchar *label)
{
  GtkAllocation alloc;
  GtkRequisition req;

  alloc.x = ICONLIST_XOFFSET;
  alloc.y = cur_y;
  alloc.width = ICONLIST_WIDTH;
  alloc.height = 1;
  gtk_widget_size_allocate (row, &alloc);
  gtk_widget_size_request (row, &req);
  alloc.height = req.height;
  gtk_widget_size_allocate (row, &alloc);
  gtk_widget_set_usize (row, alloc.width, -1);
  
  cur_y += req.height + 8;
  
  gtk_fixed_put (GTK_FIXED (programs_fixed), row, alloc.x, alloc.y);

  if (label)
    {
      GtkWidget *w;
      
      w = make_label (label, req.height);
      alloc.x -= LABEL_WIDTH;
      gtk_widget_size_request (w, &req);
      alloc.width = req.width;
      alloc.height = req.height;
      gtk_widget_size_allocate (w, &alloc);
      gtk_fixed_put (GTK_FIXED (programs_fixed), w, alloc.x, alloc.y);
      gtk_widget_show (w);
      
      labels = g_slist_prepend (labels, w);
    }

  return cur_y;
}

static void 
refresh_callback (void)
{
  GList *l;
  GSList *ll;
  int cur_y;
  GtkWidget *row;

  cur_y = 16;

  for (ll = rows; ll; ll = ll->next)
    {
      gtk_widget_destroy (ll->data);
    }
  g_slist_free (rows);
  rows = NULL;

  for (ll = labels; ll; ll = ll->next)
    {
      gtk_widget_destroy (ll->data);
    }
  g_slist_free (labels);
  rows = NULL;

  for (l = groups; l; l = l->next)
    {
      struct package_group *group = l->data;

      if (group->hide)
	continue;

      if (group->items == NULL)
	continue;

      row = create_row (group->items, group->name);
      g_object_set_data (G_OBJECT (row), "group", group);

      rows = g_slist_prepend (rows, row);

      cur_y = add_row (row, cur_y, group->name);
    }
}

gboolean
draw_programs (GtkWidget *widget, GdkEventExpose *ev)
{
  if (background)
    {
      GdkRectangle r, br;

      br.x = 0;
      br.y = 0;
      br.width = gdk_pixbuf_get_width (background);
      br.height = gdk_pixbuf_get_height (background);

      if (gdk_rectangle_intersect (&ev->area, &br, &r))
	{
	  gdk_draw_pixbuf (widget->window, 
			   widget->style->fg_gc[widget->state],
			   background,
			   r.x, r.y,
			   r.x, r.y,
			   r.width, r.height,
			   GDK_RGB_DITHER_NORMAL, 0, 0);
	}
    }
  else
    gdk_draw_rectangle (widget->window, bg_gc, TRUE,
			ev->area.x, ev->area.y,
			ev->area.width, ev->area.height);

  return FALSE;
}

void
set_bg_color (GtkWidget *w, guint32 c)
{
  GdkColormap *gcm;
  GdkColor color;

  color.red = (c & 0xff);
  color.green = (c & 0xff00) >> 8;
  color.blue = (c & 0xff0000) >> 16;

  color.red |= (color.red << 8);
  color.green |= (color.green << 8);
  color.blue |= (color.blue << 8);

  gcm = gdk_drawable_get_colormap (w->window);
  
  gdk_colormap_alloc_color (gcm, &color, FALSE, TRUE);
  
  gdk_gc_set_foreground (bg_gc, &color);
}

void
realize_programs (GtkWidget *w)
{
  bg_gc = gdk_gc_new (w->window);

  set_bg_color (w, 0xff0000);
}

GtkWidget *
build_programs (void)
{
  GtkWidget *draw;

  draw = gtk_fixed_new ();
  gtk_widget_set_name (draw, "row-widget");
  gtk_fixed_set_has_window (GTK_FIXED (draw), TRUE);

  g_signal_connect (G_OBJECT (draw), "expose_event", G_CALLBACK (draw_programs), NULL);
  g_signal_connect_after (G_OBJECT (draw), "realize", G_CALLBACK (realize_programs), NULL);

  programs_fixed = draw;

  return draw;
}

GtkWidget *
create_row_view (void)
{
  GtkWidget *progs_draw;

  progs_draw = build_programs ();
  gtk_widget_show (progs_draw);

  g_object_set_data (G_OBJECT (progs_draw), "refresh_func", refresh_callback);

  toplevel = progs_draw;

  return progs_draw;
}
