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

#include "image_tools.h"

#define WINDOW_NAME "Gallery"
#define _(_x) gettext (_x)

#define MAX_ICON_SIZE 48
#define MARGIN_X 2
#define MARGIN_Y 2

GtkWidget *window, *vbox2, *scrolled_window;
GtkWidget *dirbrowser_window;
GtkWidget *view_widget, *image_widget;
GdkPixbuf *image_pixbuf, *scaled_image_pixbuf;
GtkWidget *image_event_box;
GtkWidget *loading_toolbar, *tools_toolbar;
GtkWidget *loading_progress_bar;
GtkAdjustment *h_adjust, *v_adjust;
GList *image_filenames;

// 0 = Detailed list
// 1 = Thumbs
gint current_view = 0;
gint loading_directory = 0;
gboolean confine_pointer_to_window;

guint slideshow_timer = 0;

guint x_start, y_start, x_max, y_max;
double xadj_start, yadj_start;
guint zoom_timer_id;

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
  { "list_view", "gallery/list_view" },
  { "icon_view", "gallery/icon_view" },
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
  gint orig_width;
  gint orig_height;
  gint width;
  gint height;
  gint file_size;
  time_t file_modified;
  gchar *file_name;
} ImageInfo;

guint window_x = 240, window_y = 310;

static double starting_angle;	// for rotating drags

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

static double
angle (GtkWidget *w, int x, int y)
{
  int dx = x - (w->allocation.width / 2),
    dy = y - (w->allocation.height / 2);

  double a = atan2 ((double)dy, (double)dx);

  return a;
}

static void
pan_button_down (GtkWidget *w, GdkEventButton *b)
{
  x_start = b->x_root;
  y_start = b->y_root;

  xadj_start = gtk_adjustment_get_value (h_adjust);
  yadj_start = gtk_adjustment_get_value (v_adjust);

  x_max = (gdk_pixbuf_get_width (GDK_PIXBUF (scaled_image_pixbuf))) - (scrolled_window->allocation.width - 4);
  y_max = (gdk_pixbuf_get_height (GDK_PIXBUF (scaled_image_pixbuf))) - (scrolled_window->allocation.height - 4);

  gdk_pointer_grab (w->window, 
		    FALSE, GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK,
		    confine_pointer_to_window ? w->window : NULL, NULL, b->time);
}

static void
button_down (GtkWidget *w, GdkEventButton *b)
{
  pan_button_down (w, b);
}

static void
button_up (GtkWidget *w, GdkEventButton *b)
{
  gdk_pointer_ungrab (b->time);
}

static void
pan (GtkWidget *w, GdkEventMotion *m)
{
  gint dx = m->x_root - x_start, dy = m->y_root - y_start;
  double x, y;

  x = xadj_start - dx;
  y = yadj_start - dy;

  if (x > x_max)
    x = x_max;

  if (y > y_max)
    y = y_max;

  gtk_adjustment_set_value (h_adjust, x);
  gtk_adjustment_set_value (v_adjust, y);

  gdk_window_get_pointer (w->window, NULL, NULL, NULL);
}

static void
motion_notify (GtkWidget *w, GdkEventMotion *m, GdkPixbuf *pixbuf)
{
  pan (w, m);
}

static void
show_image (GtkWidget *widget, GList *loaded_filenames)
{
  gchar *filename;

  image_filenames = loaded_filenames;
  filename = loaded_filenames->data;

  gtk_widget_hide (view_widget);

  image_pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
  scaled_image_pixbuf = image_pixbuf;

  gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget), image_pixbuf);

  gtk_widget_show (tools_toolbar);
  gtk_widget_show (image_event_box);
  gtk_widget_show (image_widget);
  gtk_widget_show (scrolled_window);
}

static void
open_from_file (gchar *filename)
{
  GList *buf = NULL;

  buf = g_list_append (buf, (gpointer) filename);
  show_image (NULL, buf);
}

static gboolean
image_zoom_hyper (GdkPixbuf *pixbuf)
{
  gint width, height;

  width = gdk_pixbuf_get_width (GDK_PIXBUF (pixbuf));
  height = gdk_pixbuf_get_height (GDK_PIXBUF (pixbuf));

  scaled_image_pixbuf = gdk_pixbuf_scale_simple (GDK_PIXBUF (image_pixbuf), width, height, GDK_INTERP_HYPER);
  gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget), GDK_PIXBUF (scaled_image_pixbuf));

  return FALSE;
}

static void
image_rotate ()
{
  GdkPixbuf *pixbuf;

  gtk_timeout_remove (zoom_timer_id);

  if (!scaled_image_pixbuf)
    scaled_image_pixbuf = image_pixbuf;

  pixbuf = image_tools_rotate (GDK_PIXBUF (image_pixbuf), 90);
  g_object_unref (image_pixbuf);
  image_pixbuf = pixbuf;

  pixbuf = image_tools_rotate (GDK_PIXBUF (scaled_image_pixbuf), 90);
  g_object_unref (scaled_image_pixbuf);
  scaled_image_pixbuf = pixbuf;

  gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget), GDK_PIXBUF (scaled_image_pixbuf));

  zoom_timer_id = gtk_timeout_add (1000, GTK_SIGNAL_FUNC (image_zoom_hyper), scaled_image_pixbuf);
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

  if (!scaled_image_pixbuf)
    scaled_image_pixbuf = image_pixbuf;

  width = gdk_pixbuf_get_width (GDK_PIXBUF (scaled_image_pixbuf)) * 1.1;
  height = gdk_pixbuf_get_height (GDK_PIXBUF (scaled_image_pixbuf)) * 1.1;

  scaled_image_pixbuf = gdk_pixbuf_scale_simple (GDK_PIXBUF (image_pixbuf), width, height, GDK_INTERP_NEAREST);
  gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget), GDK_PIXBUF (scaled_image_pixbuf));

  zoom_timer_id = gtk_timeout_add (1000, GTK_SIGNAL_FUNC (image_zoom_hyper), scaled_image_pixbuf);
}

void
image_zoom_out ()
{
  gint width, height;

  gtk_timeout_remove (zoom_timer_id);

  if (!scaled_image_pixbuf)
    scaled_image_pixbuf = image_pixbuf;

  width = gdk_pixbuf_get_width (GDK_PIXBUF (scaled_image_pixbuf)) / 1.1;
  height = gdk_pixbuf_get_height (GDK_PIXBUF (scaled_image_pixbuf)) / 1.1;

  scaled_image_pixbuf = gdk_pixbuf_scale_simple (GDK_PIXBUF (image_pixbuf), width, height, GDK_INTERP_NEAREST);
  gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget), GDK_PIXBUF (scaled_image_pixbuf));

  zoom_timer_id = gtk_timeout_add (1000, GTK_SIGNAL_FUNC (image_zoom_hyper), scaled_image_pixbuf);

}

void
image_zoom_1 ()
{
  scaled_image_pixbuf = image_pixbuf;
  gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget), GDK_PIXBUF (scaled_image_pixbuf));
}

void
image_zoom_fit ()
{
  gint width, height;
  gint widget_width, widget_height;
  float ratio;

  if (!scaled_image_pixbuf)
    scaled_image_pixbuf = image_pixbuf;

  widget_width = image_widget->allocation.width;
  widget_height = image_widget->allocation.height - (tools_toolbar->allocation.height);

  width = gdk_pixbuf_get_width (GDK_PIXBUF (image_pixbuf));
  height = gdk_pixbuf_get_height (GDK_PIXBUF (image_pixbuf));

  if( height > width )
  {
    ratio = (float) widget_height / (float) height;
    height = widget_height;
    width = width * ratio;
  }
  else
  {
    ratio = (float) widget_width / (float) width;
    width = widget_width;
    height = height * ratio;
  }

  scaled_image_pixbuf = gdk_pixbuf_scale_simple (GDK_PIXBUF (image_pixbuf), width, height, GDK_INTERP_HYPER);
  gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget), GDK_PIXBUF (scaled_image_pixbuf));
}

static void
hide_image ()
{
  if (view_widget)
  {
    gtk_widget_hide (image_widget);
    gtk_widget_hide (image_event_box);
    gtk_widget_hide (tools_toolbar);
    gtk_widget_hide (scrolled_window);
    gtk_widget_show (view_widget);
  }
}

static gboolean
render_list_view_expose_event (GtkWidget *drawing_area, GdkEventExpose *event, GList *loaded_images)
{
  ImageInfo *image_info;
  PangoLayout *pango_title_layout, *pango_dimensions_layout, *pango_size_layout, *pango_modified_layout;
  PangoRectangle pango_title_rect, pango_dimensions_rect, pango_size_rect, pango_modified_rect;
  GList *this_item;
  gchar *buf;
  gint y = 0, text_y = 0;

  this_item = loaded_images;

  gdk_draw_rectangle (drawing_area->window, drawing_area->style->white_gc, TRUE, 0, 0, drawing_area->allocation.width, drawing_area->allocation.height);

  while (this_item)
  {
    image_info = (ImageInfo *) this_item->data;

    pango_title_layout = gtk_widget_create_pango_layout (drawing_area, NULL);
    pango_layout_set_width (pango_title_layout, -1);
    buf = g_strdup_printf ("<span size=\"small\" weight=\"bold\">%s</span>", basename (image_info->file_name));
    pango_layout_set_markup (pango_title_layout, buf, -1);
    g_free (buf);
    pango_layout_get_pixel_extents (pango_title_layout, &pango_title_rect, NULL);

    pango_dimensions_layout = gtk_widget_create_pango_layout (drawing_area, NULL);
    pango_layout_set_width (pango_dimensions_layout, -1);
    buf = g_strdup_printf ("<span size=\"x-small\" foreground=\"#555555\">Dimensions: %dx%d</span>", image_info->orig_width, image_info->orig_height);
    pango_layout_set_markup (pango_dimensions_layout, buf, -1);
    g_free (buf);
    pango_layout_get_pixel_extents (pango_dimensions_layout, &pango_dimensions_rect, NULL);

    pango_size_layout = gtk_widget_create_pango_layout (drawing_area, NULL);
    pango_layout_set_width (pango_size_layout, -1);
    buf = g_strdup_printf ("<span size=\"x-small\" foreground=\"#555555\">Size: %dk</span>", image_info->file_size);
    pango_layout_set_markup (pango_size_layout, buf, -1);
    g_free (buf);
    pango_layout_get_pixel_extents (pango_size_layout, &pango_size_rect, NULL);

    pango_modified_layout = gtk_widget_create_pango_layout (drawing_area, NULL);
    pango_layout_set_width (pango_modified_layout, -1);
    buf = g_strdup_printf ("<span size=\"x-small\" foreground=\"#555555\">Modified: %s</span>", ctime (&image_info->file_modified));
    pango_layout_set_markup (pango_modified_layout, buf, -1);
    g_free (buf);
    pango_layout_get_pixel_extents (pango_modified_layout, &pango_modified_rect, NULL);

    gdk_pixbuf_render_to_drawable (image_info->pixbuf, drawing_area->window, drawing_area->style->white_gc, 0, 0, MARGIN_X, y + MARGIN_Y, image_info->width, image_info->height, GDK_RGB_DITHER_NORMAL, MARGIN_X, y + MARGIN_Y);

    gdk_draw_layout (drawing_area->window, drawing_area->style->black_gc, (MARGIN_X * 3) + MAX_ICON_SIZE, y, pango_title_layout);
    text_y = pango_title_rect.height + MARGIN_Y;
    gdk_draw_layout (drawing_area->window, drawing_area->style->black_gc, (MARGIN_X * 3) + MAX_ICON_SIZE, y + text_y, pango_dimensions_layout);
    text_y = text_y + pango_dimensions_rect.height + MARGIN_Y;
    gdk_draw_layout (drawing_area->window, drawing_area->style->black_gc, (MARGIN_X * 3) + MAX_ICON_SIZE, y + text_y, pango_size_layout);
    text_y = text_y + pango_size_rect.height + MARGIN_Y;
    gdk_draw_layout (drawing_area->window, drawing_area->style->black_gc, (MARGIN_X * 3) + MAX_ICON_SIZE, y + text_y, pango_modified_layout);
    text_y = text_y + pango_modified_rect.height;

    if (text_y >= MAX_ICON_SIZE)
      y = y + text_y;
    else
      y = y + MAX_ICON_SIZE;

    y = y + (MARGIN_Y * 2);

    gdk_draw_line (drawing_area->window, drawing_area->style->black_gc, MARGIN_X, y, drawing_area->allocation.width - MARGIN_X, y);

    y = y + 1;

    g_object_unref (pango_title_layout);
    g_object_unref (pango_dimensions_layout);
    g_object_unref (pango_size_layout);
    g_object_unref (pango_modified_layout);

    this_item = this_item->next;
  }
  return TRUE;
}

static void
render_list_view_free (GtkObject *object, GList *loaded_images)
{
  ImageInfo *image_info;
  GList *this_item;

  this_item = loaded_images;
  while (this_item)
  {
    image_info = (ImageInfo *) this_item->data;
    g_object_unref (image_info->pixbuf);
    g_free (image_info->file_name);
    g_free (image_info);

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
  ImageInfo *image_info;
  GList *loaded_images = NULL;
  GList *this_item;
  gint num_items = g_list_length (image_filenames);
  struct stat file_stats;

  this_item = image_filenames;
  while (this_item)
  {
    image_info = g_malloc (sizeof (*image_info));
    image_info->file_name = g_strdup ((gchar *) this_item->data);
    image_info->pixbuf = gdk_pixbuf_new_from_file (image_info->file_name, NULL);
    image_info->orig_width = gdk_pixbuf_get_width (image_info->pixbuf);
    image_info->orig_height = gdk_pixbuf_get_height (image_info->pixbuf);
    image_info->width = gdk_pixbuf_get_width (image_info->pixbuf);
    image_info->height = gdk_pixbuf_get_height (image_info->pixbuf);

    stat (image_info->file_name, &file_stats);
    image_info->file_size = file_stats.st_size / 1024;
    image_info->file_modified = file_stats.st_mtime;

    if (image_info->pixbuf)
    {
      if (image_info->width > MAX_ICON_SIZE || image_info->height > MAX_ICON_SIZE)
      {
	if (image_info->width > image_info->height)
	{
	  image_info->height = image_info->height / (image_info->width / MAX_ICON_SIZE);
	  image_info->width = MAX_ICON_SIZE;
	}
        else if (image_info->height > image_info->width)
        {
	  image_info->width = image_info->width / (image_info->height / MAX_ICON_SIZE);
	  image_info->height = MAX_ICON_SIZE;
	}
        else
	{
	  image_info->width = MAX_ICON_SIZE;
	  image_info->height = MAX_ICON_SIZE;
	}

	pixbuf = gdk_pixbuf_scale_simple (image_info->pixbuf, image_info->width, image_info->height, GDK_INTERP_HYPER);
	g_object_unref (image_info->pixbuf);
	image_info->pixbuf = pixbuf;
      }

      loaded_images = g_list_append (loaded_images, image_info);
    }

    this_item = this_item->next;

    gtk_progress_bar_pulse (GTK_PROGRESS_BAR (loading_progress_bar));

    while (gtk_events_pending ())
      gtk_main_iteration ();
  }

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  gtk_widget_size_request (scrolled_window, &scrolled_window_requisition);

  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (GTK_WIDGET (drawing_area), scrolled_window_requisition.width, num_items * (MAX_ICON_SIZE + (MARGIN_Y * 2) + 1));
  g_signal_connect (G_OBJECT (drawing_area), "expose_event",  
                    G_CALLBACK (render_list_view_expose_event), loaded_images);
  g_signal_connect (G_OBJECT (scrolled_window), "destroy",  
                    G_CALLBACK (render_list_view_free), loaded_images);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), drawing_area);
  gtk_widget_show_all (scrolled_window);

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

    gpe_iconlist_add_item (GPE_ICONLIST(il), basename (buf), image_filename, (gpointer) this_item);

    gtk_progress_bar_pulse (GTK_PROGRESS_BAR (loading_progress_bar));

    while (gtk_events_pending ())
      gtk_main_iteration ();

    this_item = this_item->next;
  }

  gtk_widget_show (il);
  return il;
}

static void
render_view (GtkWidget *parent, gint view)
{
  image_filenames = g_list_first (image_filenames);

  if (image_widget)
  {
    gtk_widget_hide (image_widget);
    gtk_widget_hide (image_event_box);
    gtk_widget_hide (tools_toolbar);
    gtk_widget_hide (scrolled_window);
  }

  gtk_widget_show (loading_toolbar);

  if (view_widget)
    gtk_widget_destroy (view_widget);

  if (view == 0)
    view_widget = render_icon_view ();
  if (view == 1)
    view_widget = render_list_view ();

  gtk_signal_connect (GTK_OBJECT (view_widget), "clicked",
		      GTK_SIGNAL_FUNC (show_image), NULL);
  gtk_box_pack_start (GTK_BOX (vbox2), view_widget, TRUE, TRUE, 0);

  gtk_widget_hide (loading_toolbar);
}

static void
add_directory (gchar *directory)
{
  gchar *filename;
  struct dirent *d;
  DIR *dir;

  loading_directory = 1;
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
        filename = g_strdup_printf ("%s%s", directory, g_strdup (d->d_name));

        if (stat (filename, &s) == 0)
        {
	  if (strstr (filename, ".png") || strstr (filename, ".jpg") || strstr (filename, ".jpeg"))
	  {
	    image_filenames = g_list_append (image_filenames, (gpointer) filename);
	  }
	}
      }
    }
    render_view (NULL, current_view);
  }
}

static void
next_image ()
{
  GList *buf;

  if (image_widget)
  {
    buf = image_filenames;
    buf = g_list_next (image_filenames);

    if (buf == NULL)
      buf = g_list_first (image_filenames);

    show_image (NULL, (gpointer) buf);
  }
}

static void
previous_image ()
{
  GList *buf;

  if (image_widget)
  {
    buf = image_filenames;
    buf = g_list_previous (image_filenames);

    if (buf == NULL)
      buf = g_list_last (image_filenames);

    show_image (NULL, (gpointer) buf);
  }
}

static void
stop_slideshow ()
{
  if (slideshow_timer != 0)
  {
    gtk_timeout_remove (slideshow_timer);
  }
}

static void
start_slideshow (GtkWidget *widget, GtkWidget *spin_button)
{
  GtkWidget *parent_window;
  guint delay;

  parent_window = g_object_get_data (G_OBJECT (spin_button), "parent_window");
  delay = gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_button));

  gtk_widget_destroy (parent_window);

  if (slideshow_timer == 0)
  {
    slideshow_timer = gtk_timeout_add (delay, next_image, NULL);
  }
  else
  {
    stop_slideshow ();
  }
}

static void
show_new_slideshow (void)
{
  GtkWidget *slideshow_dialog;
  GtkWidget *spin_button, *hbox, *label1, *label2;
  GtkAdjustment *spin_button_adjustment;
  GtkWidget *start_button, *close_button;

  slideshow_dialog = gtk_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (slideshow_dialog), GTK_WINDOW (window));
  gtk_widget_realize (slideshow_dialog);
  gtk_signal_connect (GTK_OBJECT (slideshow_dialog), "destroy",
		      GTK_SIGNAL_FUNC (start_slideshow), spin_button);

  hbox = gtk_hbox_new (FALSE, 0);

  label1 = gtk_label_new ("Delay ");
  label2 = gtk_label_new (" seconds");

  spin_button_adjustment = (GtkAdjustment *) gtk_adjustment_new (3.0, 1.0, 59.0, 1.0, 1.0, 1.0);
  spin_button = gtk_spin_button_new (spin_button_adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin_button), TRUE);

  start_button = gpe_picture_button (GTK_DIALOG (slideshow_dialog)->action_area->style, _("Start"), "slideshow");
  close_button = gpe_picture_button (GTK_DIALOG (slideshow_dialog)->action_area->style, _("Cancel"), "cancel");

  g_object_set_data (G_OBJECT (spin_button), (gpointer) slideshow_dialog, "parent_window");

  gtk_signal_connect (GTK_OBJECT (start_button), "clicked",
		      GTK_SIGNAL_FUNC (start_slideshow), spin_button);
  gtk_signal_connect (GTK_OBJECT (close_button), "clicked",
		      GTK_SIGNAL_FUNC (kill_widget), slideshow_dialog);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (slideshow_dialog)->vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label1, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spin_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label2, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (slideshow_dialog)->action_area), start_button, TRUE, TRUE, 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (slideshow_dialog)->action_area), close_button, TRUE, TRUE, 4);

  gtk_widget_show (hbox);
  gtk_widget_show (slideshow_dialog);
  gtk_widget_show (label1);
  gtk_widget_show (spin_button);
  gtk_widget_show (label2);
  gtk_widget_show (start_button);
  gtk_widget_show (close_button);
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
  GtkWidget *vbox, *toolbar, *loading_label, *toolbar_icon;
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
  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);
  vbox2 = gtk_vbox_new (FALSE, 0);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_NEVER);

  h_adjust = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (scrolled_window));
  v_adjust = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_window));

  image_widget = gtk_image_new_from_pixbuf (image_pixbuf);
  image_event_box = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (image_event_box), image_widget);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), image_event_box);

  gtk_signal_connect (GTK_OBJECT (image_event_box), "button-press-event", GTK_SIGNAL_FUNC (button_down), NULL);
  gtk_signal_connect (GTK_OBJECT (image_event_box), "button-release-event", GTK_SIGNAL_FUNC (button_up), NULL);
  gtk_signal_connect (GTK_OBJECT (image_event_box), "motion-notify-event", GTK_SIGNAL_FUNC (motion_notify), NULL);

  gtk_widget_add_events (GTK_WIDGET (image_event_box), GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  loading_label = gtk_label_new ("Loading...");

  loading_progress_bar = gtk_progress_bar_new ();

#if GTK_MAJOR_VERSION < 2
  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_SPACE_LINE);
#else
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
#endif

#if GTK_MAJOR_VERSION < 2
  tools_toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (tools_toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (tools_toolbar), GTK_TOOLBAR_SPACE_LINE);
#else
  tools_toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (tools_toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (tools_toolbar), GTK_TOOLBAR_ICONS);
#endif

#if GTK_MAJOR_VERSION < 2
  loading_toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (loading_toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (loading_toolbar), GTK_TOOLBAR_SPACE_LINE);
#else
  loading_toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (loading_toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (loading_toolbar), GTK_TOOLBAR_ICONS);
#endif

  toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Open"), 
			   _("Open file"), _("Open file"), toolbar_icon, show_dirbrowser, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_GO_BACK , GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Previous"), 
			   _("Previous image"), _("Previous image"), toolbar_icon, previous_image, NULL);

  toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Next"), 
			   _("Next image"), _("Next image"), toolbar_icon, next_image, NULL);

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
			   _("Icon view"), _("Icon view"), toolbar_icon, render_view, (gpointer) 0);

  p = gpe_find_icon ("list_view");
  toolbar_icon = gtk_image_new_from_pixbuf (p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("List view"), 
			   _("List view"), _("List view"), toolbar_icon, render_view, (gpointer) 1);

  p = gpe_find_icon ("fullscreen");
  toolbar_icon = gtk_image_new_from_pixbuf (p);
  gtk_toolbar_append_item (GTK_TOOLBAR (tools_toolbar), _("Fullscreen"), 
			   _("Toggle fullscreen"), _("Toggle fullscreen"), toolbar_icon, NULL, NULL);

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
