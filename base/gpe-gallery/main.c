/*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <libintl.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <mcheck.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/render.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>
#include <gpe/dirbrowser.h>
#include <gpe/gpe-iconlist.h>
#include <gpe/spacing.h>

#include "image_tools.h"

#define WINDOW_NAME "Gallery"
#define _(_x) gettext (_x)

#define MAX_ICON_SIZE 48
#define MARGIN_X 2
#define MARGIN_Y 2

#define TOOLBAR_TIMEOUT 2500

GtkWidget *window, *main_scrolled_window, *fullscreen_window, *fullscreen_toggle_popup, *fullscreen_toolbar_popup, *vbox2;
GtkWidget *dirbrowser_window;
GtkWidget *view_widget = NULL, *image_widget = NULL;
GdkPixbuf *image_pixbuf = NULL, *scaled_image_pixbuf = NULL;
GtkWidget *image_event_box;
GtkWidget *loading_toolbar, *tools_toolbar;
GtkWidget *loading_progress_bar;
GtkAdjustment *h_adjust, *v_adjust;
GList *image_filenames = NULL;

// 0 = Detailed list
// 1 = Thumbs
gint current_view = 1;
gint loading_directory = 0;
gint fullscreen_state = 0;
gboolean confine_pointer_to_window;

guint slideshow_timer = 0;
guint fullscreen_toolbar_timer = 0;
guint fullscreen_pointer_timer = 0;

gint x_start, y_start, x_max, y_max;
gint pointer_x, pointer_y = 0;
double xadj_start, yadj_start;
guint zoom_timer_id = 0;
static gint current_rotation = 0;

static void toggle_fullscreen ();

struct gpe_icon my_icons[] = {
  { "open", "open" },
  { "left", "left" },
  { "right", "right" },
  { "slideshow", "gallery/slideshow" },
  { "fullscreen", "gallery/fullscreen" },
  { "zoom_in", "gallery/zoom_in" },
  { "zoom_out", "gallery/zoom_out" },
  { "zoom_1", "gallery/zoom_1" },
  { "zoom_fit", "gallery/zoom_fit" },
  { "pan", "gallery/pan" },
  { "rotate", "gallery/rotate" },
  { "sharpen", "gallery/sharpen" },
  { "list_view", "list-view" },
  { "icon_view", "icon-view" },
  { "blur", "gallery/blur" },
  { "ok", "ok" },
  { "cancel", "cancel" },
  { "stop", "stop" },
  { "question", "question" },
  { "error", "error" },
  { "icon", PREFIX "/share/pixmaps/gpe-gallery.png" },
  {NULL, NULL}
};

typedef struct {
  GdkPixbuf *pixbuf;
  gchar *filename;
  gint glist_position;
  gint orig_width;
  gint orig_height;
  gint width;
  gint height;
  PangoLayout *pango_layout;
  PangoRectangle pango_layout_rect;
  GdkRectangle pixbuf_rect;
  gint total_height;
} ListItem;

guint window_x = 240, window_y = 310;

static void
kill_widget (GtkWidget *parent, GtkWidget *widget)
{
  gtk_widget_destroy (widget);
}

static void
update_window_title (void)
{
  gchar *window_title;

  window_title = g_strdup_printf ("%s", WINDOW_NAME);
  gtk_window_set_title (GTK_WINDOW (window), window_title);
}

static gboolean
hide_fullscreen_toolbar ()
{
  gtk_widget_hide (fullscreen_toolbar_popup);
  fullscreen_toolbar_timer = 0;
  return FALSE;
}

static gboolean
check_fullscreen_pointer ()
{
  gint x, y;
  
  gdk_display_get_pointer (gdk_display_get_default (), NULL, &x, &y, NULL);
  
  if ((x != pointer_x) || (y != pointer_y))
  {
    if (fullscreen_toolbar_timer != 0)
    {
      g_source_remove (fullscreen_toolbar_timer);
      fullscreen_toolbar_timer = 0;
    }
    gtk_widget_show_all (fullscreen_toolbar_popup);
    gtk_widget_set_uposition (fullscreen_toolbar_popup, 0, fullscreen_window->allocation.height - fullscreen_toolbar_popup->allocation.height);
  }
  else
  {
    if (fullscreen_toolbar_timer == 0)
      fullscreen_toolbar_timer = g_timeout_add (TOOLBAR_TIMEOUT, hide_fullscreen_toolbar, NULL);
  }
  
  gdk_display_get_pointer (gdk_display_get_default (), NULL, &pointer_x, &pointer_y, NULL);
  gtk_window_present(GTK_WINDOW(fullscreen_toggle_popup));
  
  return TRUE;
}

static void
pan_button_down (GtkWidget *w, GdkEventButton *b, GtkWidget *scrolled_window)
{
  x_start = b->x_root;
  y_start = b->y_root;

  xadj_start = gtk_adjustment_get_value (gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (scrolled_window)));
  yadj_start = gtk_adjustment_get_value (gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_window)));

  x_max = (gdk_pixbuf_get_width (GDK_PIXBUF (scaled_image_pixbuf))) - (scrolled_window->allocation.width - 4);
  y_max = (gdk_pixbuf_get_height (GDK_PIXBUF (scaled_image_pixbuf))) - (scrolled_window->allocation.height - 4);

  if ((x_max > 0) && (y_max > 0))
    gdk_pointer_grab (w->window, 
		    FALSE, GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK,
		    confine_pointer_to_window ? w->window : NULL, NULL, b->time);
}

static void
button_down (GtkWidget *w, GdkEventButton *b, GtkWidget *scrolled_window)
{
  pan_button_down (w, b, scrolled_window);
}

static void
button_up (GtkWidget *w, GdkEventButton *b)
{
  gdk_pointer_ungrab (b->time);
}

static void
pan (GtkWidget *w, GdkEventMotion *m, GtkWidget *scrolled_window)
{
  gint dx = m->x_root - x_start, dy = m->y_root - y_start;
  double x, y;

  x = xadj_start - dx;
  y = yadj_start - dy;

  if (x > x_max)
    x = x_max;

  if (y > y_max)
    y = y_max;

  gtk_adjustment_set_value (gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (scrolled_window)), x);
  gtk_adjustment_set_value (gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_window)), y);

  gdk_window_get_pointer (w->window, NULL, NULL, NULL);
}

static void
motion_notify (GtkWidget *w, GdkEventMotion *m, GtkWidget *scrolled_window)
{
  pan (w, m, scrolled_window);
}

static void
show_image (GtkWidget *widget, GList *image_filename)
{
  gint width, height;
  gint widget_width, widget_height;
  float width_ratio, height_ratio;
  float scale_width_ratio, scale_height_ratio;
  GdkPixbuf *pixbuf = NULL;

  if (view_widget)
  	gtk_widget_hide (view_widget);

  image_filenames = image_filename;
  image_pixbuf = gdk_pixbuf_new_from_file ((gchar *) image_filename->data, NULL);
    
  widget_width = main_scrolled_window->allocation.width - 5;
  widget_height = main_scrolled_window->allocation.height - 5;
  if (widget_width <= 0)
	widget_width = 100;
  if (widget_height <= 0) 
	widget_height = 100;
  
  width = gdk_pixbuf_get_width (GDK_PIXBUF (image_pixbuf));
  height = gdk_pixbuf_get_height (GDK_PIXBUF (image_pixbuf));

  width_ratio = (float) width / (float) widget_width;
  height_ratio = (float) height / (float) widget_height;

  if (width_ratio >= height_ratio)
  {
    scale_width_ratio = 1;
    scale_height_ratio = (float) height_ratio / (float) width_ratio;
  }
  else
  {
    scale_height_ratio = 1;
    scale_width_ratio = (float) width_ratio / (float) height_ratio;
  }
  
  if (scaled_image_pixbuf != NULL)
    {	  
      g_object_unref(G_OBJECT(scaled_image_pixbuf));
	  scaled_image_pixbuf = NULL;
	}

  scaled_image_pixbuf = 
    gdk_pixbuf_scale_simple (GDK_PIXBUF (image_pixbuf), 
                             (int)(widget_width * scale_width_ratio), 
                             (int)(widget_height * scale_height_ratio), 
                             GDK_INTERP_BILINEAR);
  /* rotate image to current setting */
  if (current_rotation)
    {
      pixbuf = image_tools_rotate (GDK_PIXBUF (scaled_image_pixbuf), 
		                           current_rotation);
      g_object_unref (scaled_image_pixbuf);
      scaled_image_pixbuf = pixbuf;
	}
	
  gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget), 
                             GDK_PIXBUF (scaled_image_pixbuf));
  
  if (image_pixbuf != NULL)
    {
      g_object_unref (image_pixbuf);
	  image_pixbuf = NULL;
	}
  gtk_widget_show (image_event_box);
  gtk_widget_show (image_widget);
  gtk_widget_show (main_scrolled_window);
  gtk_widget_show (tools_toolbar);
}

static void
open_from_file (gchar *filename)
{
  image_filenames = NULL;
  image_filenames = g_list_append (image_filenames, (gpointer) filename);
  show_image (NULL, image_filenames);
}

static gboolean
image_zoom_hyper ()
{
  gint width, height;

  width = gdk_pixbuf_get_width (GDK_PIXBUF (scaled_image_pixbuf));
  height = gdk_pixbuf_get_height (GDK_PIXBUF (scaled_image_pixbuf));

  g_object_unref (scaled_image_pixbuf);

  image_pixbuf = gdk_pixbuf_new_from_file ((gchar *) image_filenames->data, NULL);
  scaled_image_pixbuf = gdk_pixbuf_scale_simple (GDK_PIXBUF (image_pixbuf), width, height, GDK_INTERP_BILINEAR);
  gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget), GDK_PIXBUF (scaled_image_pixbuf));
  g_object_unref (image_pixbuf);

  return FALSE;
}

static void
image_rotate () 
{
  GdkPixbuf *pixbuf;

  //gtk_timeout_remove (zoom_timer_id);
  current_rotation++;
  current_rotation = current_rotation % 4;
  pixbuf = image_tools_rotate (GDK_PIXBUF (scaled_image_pixbuf), 1);
  g_object_unref (scaled_image_pixbuf);
  gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget), GDK_PIXBUF (pixbuf));
  scaled_image_pixbuf = pixbuf;
	
  //zoom_timer_id = gtk_timeout_add (1000, image_zoom_hyper, scaled_image_pixbuf);
}

static void
image_sharpen ()
{
  image_tools_sharpen (scaled_image_pixbuf);
  gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget), GDK_PIXBUF (scaled_image_pixbuf));
}

static void
image_blur ()
{
  image_tools_blur (scaled_image_pixbuf);
  gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget), GDK_PIXBUF (scaled_image_pixbuf));
}

void
image_zoom_in ()
{
  gint width, height;

  gtk_timeout_remove (zoom_timer_id);

  width = gdk_pixbuf_get_width (GDK_PIXBUF (scaled_image_pixbuf)) * 1.1;
  height = gdk_pixbuf_get_height (GDK_PIXBUF (scaled_image_pixbuf)) * 1.1;

  g_object_unref (scaled_image_pixbuf);

  image_pixbuf = gdk_pixbuf_new_from_file ((gchar *) image_filenames->data, NULL);
  scaled_image_pixbuf = gdk_pixbuf_scale_simple (GDK_PIXBUF (image_pixbuf), width, height, GDK_INTERP_NEAREST);
  gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget), GDK_PIXBUF (scaled_image_pixbuf));
  g_object_unref (image_pixbuf);

  zoom_timer_id = gtk_timeout_add (1000, image_zoom_hyper, scaled_image_pixbuf);
}

void
image_zoom_out ()
{
  gint width, height;

  gtk_timeout_remove (zoom_timer_id);

  width = gdk_pixbuf_get_width (GDK_PIXBUF (scaled_image_pixbuf)) / 1.1;
  height = gdk_pixbuf_get_height (GDK_PIXBUF (scaled_image_pixbuf)) / 1.1;

  g_object_unref (scaled_image_pixbuf);

  image_pixbuf = gdk_pixbuf_new_from_file ((gchar *) image_filenames->data, NULL);
  scaled_image_pixbuf = gdk_pixbuf_scale_simple (GDK_PIXBUF (image_pixbuf), width, height, GDK_INTERP_NEAREST);
  gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget), GDK_PIXBUF (scaled_image_pixbuf));
  g_object_unref (image_pixbuf);

  zoom_timer_id = gtk_timeout_add (1000, image_zoom_hyper, scaled_image_pixbuf);
}

void
image_zoom_1 ()
{
  if (scaled_image_pixbuf)
    g_object_unref (scaled_image_pixbuf);
  scaled_image_pixbuf = gdk_pixbuf_new_from_file ((gchar *) image_filenames->data, NULL);
  gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget), GDK_PIXBUF (scaled_image_pixbuf));
}

void
image_zoom_fit ()
{
  gint width, height;
  gint widget_width, widget_height;
  float width_ratio, height_ratio;
  float scale_width_ratio, scale_height_ratio;

  if (scaled_image_pixbuf)
    g_object_unref (scaled_image_pixbuf);
  image_pixbuf = gdk_pixbuf_new_from_file ((gchar *) image_filenames->data, NULL);

  if (fullscreen_state == 1)
  {
    widget_width = gdk_screen_width () - 5;
    widget_height = gdk_screen_height () - 5;
  }
  else
 {
    widget_width = vbox2->allocation.width - 5;
    widget_height = vbox2->allocation.height - 5;
  }
  width = gdk_pixbuf_get_width (GDK_PIXBUF (image_pixbuf));
  height = gdk_pixbuf_get_height (GDK_PIXBUF (image_pixbuf));

  width_ratio = (float) width / (float) widget_width;
  height_ratio = (float) height / (float) widget_height;

  if (width_ratio >= height_ratio)
  {
    scale_width_ratio = 1;
    scale_height_ratio = (float) height_ratio / (float) width_ratio;
  }
  else
  {
    scale_height_ratio = 1;
    scale_width_ratio = (float) width_ratio / (float) height_ratio;
  }

  scaled_image_pixbuf = gdk_pixbuf_scale_simple (GDK_PIXBUF (image_pixbuf), widget_width * scale_width_ratio, widget_height * scale_height_ratio, GDK_INTERP_BILINEAR);
  gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget), GDK_PIXBUF (scaled_image_pixbuf));
  if (image_pixbuf)
    {
      g_object_unref (image_pixbuf);
	  image_pixbuf = NULL;
	}
}

static void
stop_slideshow ()
{
  if (slideshow_timer != 0)
  {
    gtk_timeout_remove (slideshow_timer);
    image_filenames = g_list_first (image_filenames);
    slideshow_timer = 0;
  }
}

static void
hide_image ()
{
  stop_slideshow ();
  if (view_widget)
  {
	if (scaled_image_pixbuf)
	  {
        g_object_unref (scaled_image_pixbuf);
		scaled_image_pixbuf = NULL;
	  }

    gtk_widget_hide (image_widget);
    gtk_widget_hide (image_event_box);
    gtk_widget_hide (tools_toolbar);
    gtk_widget_hide (main_scrolled_window);
    gtk_widget_show (view_widget);
  }
}

static gboolean
render_list_view_button_press_event (GtkWidget *widget, GdkEventButton *event, GList *loaded_images)
{
  GList *this_item;
  ListItem *list_item = g_malloc (sizeof (ListItem *));
  GdkPixbuf *pixbuf_with_alpha;
  char *pixels;
  guint current_y = 0;
  guint target_y = event->y;

  if (event->button != 1)
    return FALSE;

  if (loaded_images == NULL)
    return FALSE;

  this_item = loaded_images;

  while (current_y <= target_y)
  {
    list_item = (ListItem *) this_item->data;

    current_y = current_y + list_item->total_height;

    this_item = this_item->next;
  }

  pixbuf_with_alpha = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, gdk_pixbuf_get_bits_per_sample (list_item->pixbuf), list_item->pixbuf_rect.width, list_item->pixbuf_rect.height);
  pixels = gdk_pixbuf_get_pixels (pixbuf_with_alpha);
  memset (pixels, 255, list_item->pixbuf_rect.width * list_item->pixbuf_rect.height * gdk_pixbuf_get_bits_per_sample (list_item->pixbuf) * 4 / 8);
  gdk_pixbuf_composite (list_item->pixbuf, pixbuf_with_alpha, 0, 0, list_item->pixbuf_rect.width, list_item->pixbuf_rect.height, 0, 0, 1, 1, GDK_INTERP_BILINEAR, 100);

  gdk_draw_rectangle (widget->window, widget->style->white_gc, TRUE, list_item->pixbuf_rect.x, list_item->pixbuf_rect.y, list_item->pixbuf_rect.width, list_item->pixbuf_rect.height);
  gdk_pixbuf_render_to_drawable (pixbuf_with_alpha, widget->window, widget->style->white_gc, 0, 0, list_item->pixbuf_rect.x, list_item->pixbuf_rect.y, list_item->pixbuf_rect.width, list_item->pixbuf_rect.height, GDK_RGB_DITHER_NORMAL, 0, 0);

  g_object_unref (pixbuf_with_alpha);

  return TRUE;
}

static gboolean
render_list_view_button_release_event (GtkWidget *widget, GdkEventButton *event, GList *loaded_images)
{
  GList *this_item;
  ListItem *list_item = g_malloc (sizeof (ListItem *));
  guint current_y = 0;
  guint target_y = event->y;

  if (event->button != 1)
    return FALSE;

  if (loaded_images == NULL)
    return FALSE;

  this_item = loaded_images;

  while (current_y <= target_y)
  {
    list_item = (ListItem *) this_item->data;

    current_y = current_y + list_item->total_height;

    this_item = this_item->next;
  }

  show_image (NULL, g_list_nth (g_list_first (image_filenames), list_item->glist_position));
  return TRUE;
}

static gboolean
render_list_view_expose_event (GtkWidget *drawing_area, GdkEventExpose *event, GList *loaded_images)
{
  ListItem *list_item;
  GList *this_item;
  GdkRectangle pixbuf_exposed_rect, layout_rect, layout_exposed_rect;
  gint y = 0, new_y = 0;

  this_item = loaded_images;

  gdk_draw_rectangle (drawing_area->window, drawing_area->style->white_gc, TRUE, 0, 0, drawing_area->allocation.width, drawing_area->allocation.height);

  while (this_item)
  {
    list_item = (ListItem *) this_item->data;

    list_item->pixbuf_rect.x = MARGIN_X;
    list_item->pixbuf_rect.y = y + MARGIN_Y;
    list_item->pixbuf_rect.width = list_item->width;
    list_item->pixbuf_rect.height = list_item->height;

    if (gdk_rectangle_intersect (&event->area, &list_item->pixbuf_rect, &pixbuf_exposed_rect) == TRUE)
      gdk_pixbuf_render_to_drawable (list_item->pixbuf, drawing_area->window, drawing_area->style->white_gc, 0, 0, list_item->pixbuf_rect.x, list_item->pixbuf_rect.y, list_item->pixbuf_rect.width, list_item->pixbuf_rect.height, GDK_RGB_DITHER_NORMAL, 0, 0);

    layout_rect.x = (MARGIN_X * 3) + MAX_ICON_SIZE;
    layout_rect.y = y;
    layout_rect.width = list_item->pango_layout_rect.width;
    layout_rect.height = list_item->pango_layout_rect.height;

    if (gdk_rectangle_intersect (&event->area, &layout_rect, &layout_exposed_rect) == TRUE)
      gdk_draw_layout (drawing_area->window, drawing_area->style->black_gc, layout_rect.x, layout_rect.y, list_item->pango_layout);

    if (list_item->pango_layout_rect.height >= MAX_ICON_SIZE)
      new_y = list_item->pango_layout_rect.height;
    else
      new_y = MAX_ICON_SIZE;

    new_y = new_y + (MARGIN_Y * 2);

    gdk_draw_line (drawing_area->window, drawing_area->style->black_gc, MARGIN_X, y + new_y, drawing_area->allocation.width - MARGIN_X, y + new_y);

    new_y = new_y + 1;

    list_item->total_height = new_y;

    y = y + new_y;

    this_item = this_item->next;
  }
  return TRUE;
}

static void
render_list_view_free (GtkObject *object, GList *loaded_images)
{
  ListItem *list_item;
  GList *this_item;

  this_item = loaded_images;
  while (this_item)
  {
    list_item = (ListItem *) this_item->data;
    g_object_unref (list_item->pixbuf);
    g_object_unref (list_item->pango_layout);
    g_free (list_item);

    this_item = this_item->next;
  }

  g_list_free (loaded_images);
}

static GtkWidget *
render_list_view ()
{
  GtkWidget *scrolled_window, *drawing_area;
  GtkRequisition scrolled_window_requisition;
  GdkPixbuf *pixbuf;
  ListItem *list_item;
  GList *loaded_images = NULL;
  GList *this_item;
  gint num_items = g_list_length (image_filenames);
  gchar *buf;
  gint file_size;
  time_t file_modified;
  struct stat file_stats;

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), 
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  gtk_widget_size_request (scrolled_window, &scrolled_window_requisition);

  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (GTK_WIDGET (drawing_area), 
                               scrolled_window_requisition.width - 1, 
                               num_items * (MAX_ICON_SIZE + (MARGIN_Y * 2) + 1));

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), drawing_area);
  gtk_widget_show_all (scrolled_window);

  this_item = image_filenames;
  while (this_item)
  {
    list_item = g_malloc (sizeof (*list_item));
    list_item->filename = (gchar *) this_item->data;
    list_item->glist_position = g_list_position (image_filenames, this_item);
    list_item->pixbuf = gdk_pixbuf_new_from_file (list_item->filename, NULL);
    list_item->orig_width = gdk_pixbuf_get_width (list_item->pixbuf);
    list_item->orig_height = gdk_pixbuf_get_height (list_item->pixbuf);
    list_item->width = gdk_pixbuf_get_width (list_item->pixbuf);
    list_item->height = gdk_pixbuf_get_height (list_item->pixbuf);

    stat (list_item->filename, &file_stats);
    file_size = file_stats.st_size / 1024;
    file_modified = file_stats.st_mtime;

    if (list_item->pixbuf)
    {
      if (list_item->width > MAX_ICON_SIZE || list_item->height > MAX_ICON_SIZE)
      {
	if (list_item->width > list_item->height)
	{
	  list_item->height = list_item->height / (list_item->width / MAX_ICON_SIZE);
	  list_item->width = MAX_ICON_SIZE;
	}
        else if (list_item->height > list_item->width)
        {
	  list_item->width = list_item->width / (list_item->height / MAX_ICON_SIZE);
	  list_item->height = MAX_ICON_SIZE;
	}
        else
	{
	  list_item->width = MAX_ICON_SIZE;
	  list_item->height = MAX_ICON_SIZE;
	}

	pixbuf = gdk_pixbuf_scale_simple (list_item->pixbuf, list_item->width, list_item->height, GDK_INTERP_BILINEAR);
	g_object_unref (list_item->pixbuf);
	list_item->pixbuf = pixbuf;
      }
    

    list_item->pango_layout = gtk_widget_create_pango_layout (drawing_area, NULL);
    pango_layout_set_width (list_item->pango_layout, -1);
    buf = g_strdup_printf ("<span size=\"small\" weight=\"bold\">%s</span>\n<span size=\"x-small\" foreground=\"#555555\">Dimensions: %dx%d</span>\n<span size=\"x-small\" foreground=\"#555555\">Size: %dk</span>\n<span size=\"x-small\" foreground=\"#555555\">Modified: %s</span>", basename (list_item->filename), list_item->orig_width, list_item->orig_height, file_size, ctime (&file_modified));
    pango_layout_set_markup (list_item->pango_layout, buf, -1);
    g_free (buf);
    pango_layout_get_pixel_extents (list_item->pango_layout, &list_item->pango_layout_rect, NULL);

    loaded_images = g_list_append (loaded_images, list_item);

    this_item = this_item->next;

    gtk_progress_bar_pulse (GTK_PROGRESS_BAR (loading_progress_bar));

    while (gtk_events_pending ())
      gtk_main_iteration ();
   }
  }

  g_signal_connect (G_OBJECT (drawing_area), "expose_event", 
                    G_CALLBACK (render_list_view_expose_event), loaded_images);
  g_signal_connect (G_OBJECT (drawing_area), "button_press_event", 
                    G_CALLBACK (render_list_view_button_press_event), loaded_images);
  g_signal_connect (G_OBJECT (drawing_area), "button_release_event", 
                    G_CALLBACK (render_list_view_button_release_event), loaded_images);
  g_signal_connect (G_OBJECT (scrolled_window), "destroy",
                    G_CALLBACK (render_list_view_free), loaded_images);

  gtk_widget_add_events (GTK_WIDGET (drawing_area), GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  return scrolled_window;
}

static GtkWidget *
render_icon_view ()
{
  GtkWidget *il;
  GList *this_item;
  gchar *image_filename, *buf;

  il = gpe_iconlist_new ();

  this_item = image_filenames;
  while (this_item)
  {
    image_filename = (gchar *) this_item->data;
    buf = g_strdup (image_filename);

    gpe_iconlist_add_item (GPE_ICONLIST(il), basename (buf), image_filename, this_item);

    gtk_progress_bar_pulse (GTK_PROGRESS_BAR (loading_progress_bar));

    while (gtk_events_pending ())
      gtk_main_iteration ();

    this_item = this_item->next;
  }

  gtk_signal_connect (GTK_OBJECT (il), "clicked",
		      GTK_SIGNAL_FUNC (show_image), NULL);
  gtk_widget_show (il);
  return il;
}

static void
render_view (GtkWidget *parent, gint view)
{
  image_filenames = g_list_first (image_filenames);

  if (image_widget)
  {
    if (scaled_image_pixbuf)
      {
        g_object_unref (scaled_image_pixbuf);
        scaled_image_pixbuf = NULL;
      }

    gtk_widget_hide (image_widget);
    gtk_widget_hide (image_event_box);
    gtk_widget_hide (tools_toolbar);
    gtk_widget_hide (main_scrolled_window);
  }

  gtk_widget_show (loading_toolbar);

  if (view_widget)
    gtk_widget_destroy (view_widget);

  if (view == 0)
    view_widget = render_icon_view ();
  if (view == 1)
    view_widget = render_list_view ();

  gtk_box_pack_start (GTK_BOX (vbox2), view_widget, TRUE, TRUE, 0);
  gtk_widget_hide (loading_toolbar);
}

static
gboolean is_image_file(const gchar* filename)
{
  gchar *fn = g_utf8_strdown(filename,-1);
  gboolean result = FALSE;

  if (strstr (fn, ".png")) 
	  result = TRUE;
  if (strstr (fn, ".jpg") || strstr (filename, ".jpeg"))
	  result = TRUE;
  if (strstr (fn, ".gif"))
	  result = TRUE;
  if (strstr (fn, ".ico"))
	  result = TRUE;
  if (strstr (fn, ".bmp"))
	  result = TRUE;
  if (strstr (fn, ".xpm"))
	  result = TRUE;
  g_free(fn);
  
  return result;
}

static void
add_directory (gchar *directory)
{
  gchar *filename;
  GList *tl; 
  struct dirent *d;
  DIR *dir;

  loading_directory = 1;
  current_rotation = 0;
  
  for (tl=g_list_first(image_filenames);tl;tl=g_list_next(tl))
	  g_free(tl->data);
  
  g_list_free (image_filenames);
  image_filenames = NULL;

  dir = opendir (directory);
  if (dir)
  {
    while (d = readdir (dir), d != NULL)
    {
      if (loading_directory == 0)
        break;

      if (d->d_name[0] != '.')
      {
        struct stat s;
        filename = g_strdup_printf ("%s%s", directory, d->d_name);

        if (stat (filename, &s) == 0)
        {
	      if (is_image_file(filename))
	      {
	        image_filenames = g_list_append (image_filenames, (gpointer) filename);
          }
	  
	    }
      }
    }
    render_view (NULL, current_view);
  }
}

static gboolean
next_image (gpointer data)
{
  if ((image_widget) && (image_filenames))
  {
    if (g_list_next (image_filenames) != NULL)
      image_filenames = g_list_next (image_filenames);
    else
      image_filenames = g_list_first (image_filenames);

    show_image (NULL, image_filenames);

    return TRUE;
  }
  else
    return FALSE;
}

static gboolean
previous_image ()
{
  if ((image_widget) && (image_filenames))
  {
    if (g_list_previous (image_filenames) != NULL)
      image_filenames = g_list_previous (image_filenames);
    else
      image_filenames = g_list_last (image_filenames);

    show_image (NULL, image_filenames);

    return TRUE;
  }
  else
    return FALSE;
}

static void
start_slideshow (GtkWidget *widget, GtkWidget *spin_button)
{
  if (slideshow_timer == 0)
  {
    image_filenames = g_list_first (image_filenames);
    show_image (NULL, image_filenames);
    slideshow_timer = gtk_timeout_add (gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_button)) * 1000, next_image, NULL);
    toggle_fullscreen ();
    }
  else
  {
    stop_slideshow ();
  }

  gtk_widget_destroy (GTK_WIDGET (g_object_get_data (G_OBJECT (widget), "slideshow_dialog")));
}

static void
show_new_slideshow (void)
{
  GtkWidget *slideshow_dialog;
  GtkWidget *spin_button, *vbox, *hbox, *hbox2, *hsep, *label1, *label2;
  GtkAdjustment *spin_button_adjustment;
  GtkWidget *start_button, *close_button;

  /* ignore if no images available */
  #warning todo: disable button in this case
  if (!image_filenames)
	 return;

  hide_image ();

  slideshow_dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_transient_for (GTK_WINDOW (slideshow_dialog), GTK_WINDOW (window));
  gtk_widget_realize (slideshow_dialog);

  vbox = gtk_vbox_new (FALSE, gpe_get_boxspacing ());
  gtk_container_set_border_width (GTK_CONTAINER (vbox), gpe_get_boxspacing ());
  hbox = gtk_hbox_new (FALSE, gpe_get_boxspacing ());
  hbox2 = gtk_hbox_new (FALSE, gpe_get_boxspacing ());

  hsep = gtk_hseparator_new ();

  label1 = gtk_label_new ("Delay ");
  label2 = gtk_label_new (" seconds");

  spin_button_adjustment = (GtkAdjustment *) gtk_adjustment_new (3.0, 1.0, 59.0, 1.0, 1.0, 1.0);
  spin_button = gtk_spin_button_new (spin_button_adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin_button), TRUE);

  start_button = gpe_picture_button (hbox2->style, _("Start"), "slideshow");
  close_button = gpe_button_new_from_stock (GTK_STOCK_CLOSE, GPE_BUTTON_TYPE_BOTH);

  g_object_set_data (G_OBJECT (start_button), "slideshow_dialog", slideshow_dialog);

  g_signal_connect (G_OBJECT (start_button), "clicked",
		      G_CALLBACK (start_slideshow), spin_button);
  g_signal_connect (G_OBJECT (close_button), "clicked",
		      G_CALLBACK (kill_widget), slideshow_dialog);

  gtk_container_add (GTK_CONTAINER (slideshow_dialog), vbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hsep, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label1, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spin_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label2, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), close_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), start_button, TRUE, TRUE, 0);

  gtk_window_set_default_size (GTK_WINDOW (slideshow_dialog), 1, 1);

  gtk_widget_show (vbox);
  gtk_widget_show (hbox);
  gtk_widget_show (hbox2);
  gtk_widget_show (label1);
  gtk_widget_show (spin_button);
  gtk_widget_show (label2);
  gtk_widget_show (hsep);
  gtk_widget_show (start_button);
  gtk_widget_show (close_button);
  gtk_widget_show (slideshow_dialog);
}

static void
toggle_fullscreen ()
{
  GtkWidget *scrolled_window, *image_widget, *image_event_box, *close_button, *fullscreen_toolbar, *toolbar_icon;
  GdkPixbuf *p;
	
  if (fullscreen_state == 1)
  {
    hide_image ();
    gtk_widget_reparent (main_scrolled_window, vbox2);
    show_image (NULL, image_filenames);

    if (fullscreen_toolbar_timer) 
	  g_source_remove (fullscreen_toolbar_timer);
    g_source_remove (fullscreen_pointer_timer);
    
    gtk_widget_destroy (fullscreen_window);
    gtk_widget_destroy (fullscreen_toggle_popup);
    gtk_widget_destroy (fullscreen_toolbar_popup);

    fullscreen_state = 0;
    fullscreen_toolbar_timer = 0;
    fullscreen_pointer_timer = 0;

    image_zoom_fit ();
  }
  else if (fullscreen_state == 0)
  {
    fullscreen_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), 
                                    GTK_POLICY_NEVER, GTK_POLICY_NEVER);

    image_widget = gtk_image_new_from_pixbuf (scaled_image_pixbuf);
    image_event_box = gtk_event_box_new ();
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), image_event_box);

    gtk_signal_connect (GTK_OBJECT (image_event_box), "button-press-event", GTK_SIGNAL_FUNC (button_down), scrolled_window);
    gtk_signal_connect (GTK_OBJECT (image_event_box), "button-release-event", GTK_SIGNAL_FUNC (button_up), NULL);
    gtk_signal_connect (GTK_OBJECT (image_event_box), "motion-notify-event", GTK_SIGNAL_FUNC (motion_notify), scrolled_window);

    gtk_widget_add_events (GTK_WIDGET (image_event_box), GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

    gtk_container_add (GTK_CONTAINER (image_event_box), image_widget);

    gtk_widget_reparent (main_scrolled_window, fullscreen_window);

    gtk_widget_show_all (fullscreen_window);

    gdk_window_fullscreen (GDK_WINDOW (fullscreen_window->window));

    fullscreen_toggle_popup = gtk_window_new (GTK_WINDOW_POPUP);
	
    close_button = gpe_button_new_from_stock (GTK_STOCK_CLOSE, GPE_BUTTON_TYPE_ICON);
    g_signal_connect (G_OBJECT (close_button), "clicked",
		      G_CALLBACK (toggle_fullscreen), NULL);
    gtk_container_add (GTK_CONTAINER (fullscreen_toggle_popup), close_button);
    gtk_widget_show_all (fullscreen_toggle_popup);

  fullscreen_toolbar_popup = gtk_window_new (GTK_WINDOW_POPUP);

  fullscreen_toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (fullscreen_toolbar), GTK_ORIENTATION_HORIZONTAL);

  toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_GO_BACK , GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (fullscreen_toolbar), _("Previous"), 
			   _("Previous image"), _("Previous image"), toolbar_icon, GTK_SIGNAL_FUNC (previous_image), NULL);

  toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (fullscreen_toolbar), _("Next"), 
			   _("Next image"), _("Next image"), toolbar_icon, GTK_SIGNAL_FUNC (next_image), NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (fullscreen_toolbar));

  p = gpe_find_icon ("zoom_in");
  toolbar_icon = gtk_image_new_from_pixbuf (p);
  gtk_toolbar_append_item (GTK_TOOLBAR (fullscreen_toolbar), _("Zoom In"), 
			   _("Zoom In"), _("Zoom In"), toolbar_icon, image_zoom_in, NULL);

  p = gpe_find_icon ("zoom_out");
  toolbar_icon = gtk_image_new_from_pixbuf (p);
  gtk_toolbar_append_item (GTK_TOOLBAR (fullscreen_toolbar), _("Zoom Out"), 
			   _("Zoom Out"), _("Zoom Out"), toolbar_icon, image_zoom_out, NULL);

  p = gpe_find_icon ("zoom_1");
  toolbar_icon = gtk_image_new_from_pixbuf (p);
  gtk_toolbar_append_item (GTK_TOOLBAR (fullscreen_toolbar), _("Zoom 1:1"), 
			   _("Zoom 1:1"), _("Zoom 1:1"), toolbar_icon, image_zoom_1, NULL);

  p = gpe_find_icon ("zoom_fit");
  toolbar_icon = gtk_image_new_from_pixbuf (p);
  gtk_toolbar_append_item (GTK_TOOLBAR (fullscreen_toolbar), _("Zoom to Fit"), 
			   _("Zoom to Fit"), _("Zoom to Fit"), toolbar_icon, image_zoom_fit, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (fullscreen_toolbar));

  p = gpe_find_icon ("rotate");
  toolbar_icon = gtk_image_new_from_pixbuf (p);
  gtk_toolbar_append_item (GTK_TOOLBAR (fullscreen_toolbar), _("Rotate"), 
			   _("Toggle Rotate"), _("Toggle Rotate"), toolbar_icon, image_rotate, NULL);

    gtk_container_add (GTK_CONTAINER (fullscreen_toolbar_popup), fullscreen_toolbar);
    gtk_widget_show_all (fullscreen_toolbar_popup);
    gtk_widget_grab_focus (toolbar_icon);
    fullscreen_pointer_timer = g_timeout_add (200, check_fullscreen_pointer, NULL);
    gtk_widget_set_uposition (fullscreen_toolbar_popup, 0, fullscreen_window->allocation.height - fullscreen_toolbar_popup->allocation.height);

    fullscreen_state = 1;

    
    image_zoom_fit ();
  }
}

static void
show_dirbrowser (void)
{
  if (!dirbrowser_window)
  {
    dirbrowser_window = gpe_create_dir_browser (_("Select directory"), (gchar *) g_get_home_dir (), GTK_SELECTION_SINGLE, add_directory);
    gtk_signal_connect (GTK_OBJECT (dirbrowser_window), "destroy", GTK_SIGNAL_FUNC (gtk_widget_destroyed), &dirbrowser_window);
    gtk_window_set_transient_for (GTK_WINDOW (dirbrowser_window), GTK_WINDOW (window));
  }
  if (!GTK_WIDGET_VISIBLE (dirbrowser_window))
    gtk_widget_show(dirbrowser_window);
}

int
main (int argc, char *argv[])
{
  GtkWidget *vbox, *scrolled_window, *toolbar, *loading_label, *toolbar_icon;
  GdkPixbuf *p;
  GdkPixmap *pmap;
  GdkBitmap *bmap;
  struct stat arg_stat;

  mtrace ();

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");
  
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  update_window_title ();
  gtk_window_set_default_size (GTK_WINDOW (window), window_x, window_y);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);
  vbox2 = gtk_vbox_new (FALSE, 0);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  main_scrolled_window = scrolled_window;

  image_widget = gtk_image_new_from_pixbuf (image_pixbuf);
  image_event_box = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (image_event_box), image_widget);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), image_event_box);

  gtk_signal_connect (GTK_OBJECT (image_event_box), "button-press-event", GTK_SIGNAL_FUNC (button_down), scrolled_window);
  gtk_signal_connect (GTK_OBJECT (image_event_box), "button-release-event", GTK_SIGNAL_FUNC (button_up), NULL);
  gtk_signal_connect (GTK_OBJECT (image_event_box), "motion-notify-event", GTK_SIGNAL_FUNC (motion_notify), scrolled_window);

  gtk_widget_add_events (GTK_WIDGET (image_event_box), GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  loading_label = gtk_label_new ("Loading...");

  loading_progress_bar = gtk_progress_bar_new ();

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);

  tools_toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (tools_toolbar), GTK_ORIENTATION_HORIZONTAL);

  loading_toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (loading_toolbar), GTK_ORIENTATION_HORIZONTAL);

  toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Open"), 
			   _("Open file"), _("Open file"), toolbar_icon, show_dirbrowser, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_GO_BACK , GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Previous"), 
			   _("Previous image"), _("Previous image"), toolbar_icon, GTK_SIGNAL_FUNC (previous_image), NULL);

  toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Next"), 
			   _("Next image"), _("Next image"), toolbar_icon, GTK_SIGNAL_FUNC (next_image), NULL);

  toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Close image"), 
			   _("Close image"), _("Close image"), toolbar_icon, hide_image, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  p = gpe_find_icon ("slideshow");
  toolbar_icon = gtk_image_new_from_pixbuf (p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Slideshow"), 
			   _("Slideshow"), _("Slideshow"), toolbar_icon, show_new_slideshow, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  p = gpe_find_icon ("icon_view");
  toolbar_icon = gtk_image_new_from_pixbuf (p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Icon view"), 
			   _("Icon view"), _("Icon view"), toolbar_icon, GTK_SIGNAL_FUNC (render_view), (gpointer) 0);

  p = gpe_find_icon ("list_view");
  toolbar_icon = gtk_image_new_from_pixbuf (p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("List view"), 
			   _("List view"), _("List view"), toolbar_icon, GTK_SIGNAL_FUNC (render_view), (gpointer) 1);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_QUIT,
			    _("Exit"), _("Tap here to exit the program"),
			    G_CALLBACK (gtk_main_quit), NULL, -1);

  p = gpe_find_icon ("fullscreen");
  toolbar_icon = gtk_image_new_from_pixbuf (p);
  gtk_toolbar_append_item (GTK_TOOLBAR (tools_toolbar), _("Fullscreen"), 
			   _("Toggle fullscreen"), _("Toggle fullscreen"), toolbar_icon, toggle_fullscreen, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (tools_toolbar));

  p = gpe_find_icon ("zoom_in");
  toolbar_icon = gtk_image_new_from_pixbuf (p);
  gtk_toolbar_append_item (GTK_TOOLBAR (tools_toolbar), _("Zoom In"), 
			   _("Zoom In"), _("Zoom In"), toolbar_icon, image_zoom_in, NULL);

  p = gpe_find_icon ("zoom_out");
  toolbar_icon = gtk_image_new_from_pixbuf (p);
  gtk_toolbar_append_item (GTK_TOOLBAR (tools_toolbar), _("Zoom Out"), 
			   _("Zoom Out"), _("Zoom Out"), toolbar_icon, image_zoom_out, NULL);

  p = gpe_find_icon ("zoom_1");
  toolbar_icon = gtk_image_new_from_pixbuf (p);
  gtk_toolbar_append_item (GTK_TOOLBAR (tools_toolbar), _("Zoom 1:1"), 
			   _("Zoom 1:1"), _("Zoom 1:1"), toolbar_icon, image_zoom_1, NULL);

  p = gpe_find_icon ("zoom_fit");
  toolbar_icon = gtk_image_new_from_pixbuf (p);
  gtk_toolbar_append_item (GTK_TOOLBAR (tools_toolbar), _("Zoom to Fit"), 
			   _("Zoom to Fit"), _("Zoom to Fit"), toolbar_icon, image_zoom_fit, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (tools_toolbar));

  p = gpe_find_icon ("sharpen");
  toolbar_icon = gtk_image_new_from_pixbuf (p);
  gtk_toolbar_append_item (GTK_TOOLBAR (tools_toolbar), _("Sharpen"), 
			   _("Sharpen"), _("Sharpen"), toolbar_icon, image_sharpen, NULL);

  p = gpe_find_icon ("blur");
  toolbar_icon = gtk_image_new_from_pixbuf (p);
  gtk_toolbar_append_item (GTK_TOOLBAR (tools_toolbar), _("Blur"), 
			   _("Blur"), _("Blur"), toolbar_icon, image_blur, NULL);

  p = gpe_find_icon ("rotate");
  toolbar_icon = gtk_image_new_from_pixbuf (p);
  gtk_toolbar_append_item (GTK_TOOLBAR (tools_toolbar), _("Rotate"), 
			   _("Toggle Rotate"), _("Toggle Rotate"), toolbar_icon, image_rotate, NULL);

  gtk_toolbar_append_widget (GTK_TOOLBAR (loading_toolbar), loading_label,
			   _("Loading..."), _("Loading..."));

  gtk_toolbar_append_widget (GTK_TOOLBAR (loading_toolbar), loading_progress_bar,
			   _("Progress"), _("Progress"));

  p = gpe_find_icon ("stop");
  toolbar_icon = gtk_image_new_from_pixbuf (p);
  gtk_toolbar_append_item (GTK_TOOLBAR (loading_toolbar), _("Stop Loading"), 
			   _("Stop Loading"), _("Stop Loading"), toolbar_icon, NULL, NULL);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), vbox2, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), scrolled_window, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), loading_toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), tools_toolbar, FALSE, FALSE, 0);

  if (gpe_find_icon_pixmap ("icon", &pmap, &bmap))
    gdk_window_set_icon (window->window, NULL, pmap, bmap);

  gtk_widget_show (window);
  gtk_widget_show (vbox);
  gtk_widget_show (vbox2);
  gtk_widget_show (toolbar);
  gtk_widget_show (loading_label);
  gtk_widget_show (loading_progress_bar);

  // Ugly hack so we know how to resize images that want to fit the screen
  gtk_widget_show (main_scrolled_window);
    while (gtk_events_pending ())
      gtk_main_iteration ();
  gtk_widget_hide (main_scrolled_window);
    while (gtk_events_pending ())
      gtk_main_iteration ();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(main_scrolled_window), 
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  if (argc > 1)
  {
    stat (argv[1], &arg_stat);

    if (S_ISREG (arg_stat.st_mode))
    {
      open_from_file (argv[1]);
      gtk_widget_hide (toolbar);
    }
    else if (S_ISDIR (arg_stat.st_mode))
    {
      add_directory (g_strdup_printf ("%s/", argv[1]));
    }
  }

  gtk_main();
  return 0;
}
