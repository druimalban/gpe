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

#include <gtk/gtk.h>

#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/render.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>
#include <gpe/dirbrowser.h>
#include <gpe/gpe-iconlist.h>

#define WINDOW_NAME "Gallery"
#define _(_x) gettext (_x)
GtkWidget *window, *vbox2;
GtkWidget *dirbrowser_window;
GtkWidget *view_widget, *image_widget, *image_pixbuf, *scaled_image_pixbuf;
GtkWidget *loading_toolbar, *tools_toolbar;
GtkWidget *loading_progress_bar;
GList *image_filenames;

// 0 = Detailed list
// 1 = Thumbs
gint current_view = 0;
gint loading_directory = 0;

guint slideshow_timer = 0;

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
  { "ok", "ok" },
  { "cancel", "cancel" },
  { "stop", "stop" },
  { "question", "question" },
  { "error", "error" },
  { "icon", PREFIX "/share/pixmaps/gpe-gallery.png" },
  {NULL, NULL}
};

guint window_x = 240, window_y = 310;

static void
update_window_title (void)
{
  gchar *window_title;

  window_title = g_strdup_printf ("%s", WINDOW_NAME);
  gtk_window_set_title (GTK_WINDOW (window), window_title);
}

void
show_image (GtkWidget *widget, gpointer *udata)
{
  gchar *filename;

  image_filenames = udata;
  filename = ((GList *) udata)->data;

  gtk_widget_hide (view_widget);

  image_pixbuf = gdk_pixbuf_new_from_file (filename, NULL);

  image_widget = gtk_image_new_from_pixbuf (image_pixbuf);
  gtk_box_pack_start (GTK_BOX (vbox2), image_widget, TRUE, TRUE, 0);

  gtk_widget_show (tools_toolbar);
  gtk_widget_show (image_widget);

  printf ("You just clicked on an image called %s\n", filename);
}

void
image_zoom_in ()
{
  gint width, height;

  if (!scaled_image_pixbuf)
    scaled_image_pixbuf = image_pixbuf;

  width = gdk_pixbuf_get_width (GDK_PIXBUF (scaled_image_pixbuf)) * 1.5;
  height = gdk_pixbuf_get_height (GDK_PIXBUF (scaled_image_pixbuf)) * 1.5;

  scaled_image_pixbuf = gdk_pixbuf_scale_simple (GDK_PIXBUF (image_pixbuf), width, height, GDK_INTERP_BILINEAR);
  gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget), GDK_PIXBUF (scaled_image_pixbuf));
}

void
image_zoom_out ()
{
  gint width, height;

  if (!scaled_image_pixbuf)
    scaled_image_pixbuf = image_pixbuf;

  width = gdk_pixbuf_get_width (GDK_PIXBUF (scaled_image_pixbuf)) / 1.5;
  height = gdk_pixbuf_get_height (GDK_PIXBUF (scaled_image_pixbuf)) / 1.5;

  scaled_image_pixbuf = gdk_pixbuf_scale_simple (GDK_PIXBUF (image_pixbuf), width, height, GDK_INTERP_BILINEAR);
  gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget), GDK_PIXBUF (scaled_image_pixbuf));
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
  float ratio;

  if (!scaled_image_pixbuf)
    scaled_image_pixbuf = image_pixbuf;

  width = gdk_pixbuf_get_width (GDK_PIXBUF (image_pixbuf));
  height = gdk_pixbuf_get_height (GDK_PIXBUF (image_pixbuf));

  if (height > width)
  {
    if (height < 200)
    {
      ratio = (float) 200 / (float) height;
      height = 200;
      width = ratio * width;
    }
    else
    {
      ratio = (float) height / (float) 200;
      height = 200;
      width = width / ratio;
    }
  }
  else
  {
    if (width < 240)
    {
      ratio = (float) 240 / (float) width;
      width = 240;
      height = ratio * height;
    }
    else
    {
      ratio = (float) width / (float) 240;
      width = 240;
      height = height / ratio;
    }
  }

  scaled_image_pixbuf = gdk_pixbuf_scale_simple (GDK_PIXBUF (image_pixbuf), width, height, GDK_INTERP_BILINEAR);
  gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget), GDK_PIXBUF (scaled_image_pixbuf));
}

void
hide_image ()
{
  if (view_widget)
  {
    gtk_widget_destroy (image_widget);
    gtk_widget_hide (tools_toolbar);
    gtk_widget_show (view_widget);
  }
}

void
enable_fullscreen ()
{
  //gdk_window_fullscreen (GDK_WINDOW (window));
}

void
disable_fullscreen ()
{
  //gdk_window_unfullscreen (GDK_WINDOW (window));
}

GtkWidget *
render_images ()
{
  GtkWidget *il;
  GList *this_item;
  gchar *image_filename, *buf;

  il = gpe_iconlist_new ();
  printf ("Starting rendering...\n");
  this_item = image_filenames;
  while (this_item)
  {
    image_filename = (gchar *) this_item->data;
    buf = g_strdup (image_filename);

    printf ("Rendering image %s\n", image_filename);

    gpe_iconlist_add_item (GPE_ICONLIST(il), basename (buf), image_filename, (gpointer) this_item);

    gtk_progress_bar_pulse (GTK_PROGRESS_BAR (loading_progress_bar));

    while (gtk_events_pending ())
      gtk_main_iteration ();

    this_item = this_item->next;
  }

  gtk_widget_show (il);
  return il;
}

void
add_directory (gchar *directory)
{
  gchar *filename, *imagename, *buf;
  struct dirent *d;
  DIR *dir;

  loading_directory = 1;
  printf ("Selected directory: %s\n", directory);
  g_list_free (image_filenames);

  if (image_widget)
  {
    gtk_widget_destroy (image_widget);
    gtk_widget_hide (tools_toolbar);
  }

  gtk_widget_show (loading_toolbar);

  dir = opendir (directory);
  if (dir)
  {
    if (view_widget)
      gtk_widget_destroy (view_widget);

    while (d = readdir (dir), d != NULL)
    {
      if (loading_directory == 0)
        break;

      if (d->d_name[0] != '.')
      {
        struct stat s;
        filename = g_strdup_printf ("%s%s", directory, g_strdup (d->d_name));
	printf ("Seen %s\n", filename);

        if (stat (filename, &s) == 0)
        {
	  if (strstr (filename, ".png") || strstr (filename, ".jpg"))
	  {
	    //printf ("Added %s\n", filename);
	    image_filenames = g_list_append (image_filenames, (gpointer) filename);
	  }
	}
      }
    }
    view_widget = render_images (image_filenames);
    gtk_signal_connect (GTK_OBJECT (view_widget), "clicked",
		        GTK_SIGNAL_FUNC (show_image), NULL);
    gtk_box_pack_start (GTK_BOX (vbox2), view_widget, TRUE, TRUE, 0);

    gtk_widget_hide (loading_toolbar);
  }
}

void
next_image ()
{
  GList *buf;

  if (image_widget)
  {
    buf = image_filenames;
    gtk_widget_destroy (image_widget);
    buf = g_list_next (image_filenames);

    if (buf == NULL)
      buf = g_list_first (image_filenames);

    show_image (NULL, (gpointer) buf);
  }
}

void
previous_image ()
{
  GList *buf;

  if (image_widget)
  {
    buf = image_filenames;
    gtk_widget_destroy (image_widget);
    buf = g_list_previous (image_filenames);

    if (buf == NULL)
      buf = g_list_last (image_filenames);

    show_image (NULL, (gpointer) buf);
  }
}

void
stop_slideshow ()
{
  if (slideshow_timer != 0)
  {
    gtk_timeout_remove (slideshow_timer);
  }
}

void
start_slideshow (GtkWidget *widget, GtkWidget *spin_button)
{
  guint delay;

  delay = gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_button));

  if (slideshow_timer == 0)
  {
    slideshow_timer = gtk_timeout_add (delay, next_image, NULL);
  }
  else
  {
    stop_slideshow ();
  }
}

void
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

  gtk_signal_connect (GTK_OBJECT (start_button), "destroy",
		      GTK_SIGNAL_FUNC (start_slideshow), spin_button);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (slideshow_dialog)->vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label1, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spin_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label2, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (slideshow_dialog)->action_area), start_button, TRUE, TRUE, 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (slideshow_dialog)->action_area), close_button, TRUE, TRUE, 4);

  gtk_widget_show (slideshow_dialog);
  gtk_widget_show (hbox);
  gtk_widget_show (label1);
  gtk_widget_show (spin_button);
  gtk_widget_show (label2);
  gtk_widget_show (start_button);
  gtk_widget_show (close_button);
}

void
show_dirbrowser (void)
{
  if (!dirbrowser_window)
  {
    dirbrowser_window = gpe_create_dir_browser (_("Select directory"), g_get_home_dir (), GTK_SELECTION_SINGLE, add_directory);
    gtk_signal_connect (GTK_OBJECT (dirbrowser_window), "destroy", GTK_SIGNAL_FUNC (gtk_widget_destroyed), &dirbrowser_window);
    gtk_window_set_transient_for (GTK_WINDOW (dirbrowser_window), GTK_WINDOW (window));
  }
  if (!GTK_WIDGET_VISIBLE (dirbrowser_window))
    gtk_widget_show(dirbrowser_window);
}

int
main (int argc, char *argv[])
{
  GtkWidget *vbox, *hbox, *toolbar, *info_label, *loading_label;
  GtkWidget *view_option_menu, *view_menu, *view_menu_item;
  GdkPixbuf *p;
  GtkWidget *pw;
  GdkPixmap *pmap;
  GdkBitmap *bmap;

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

  hbox = gtk_hbox_new (FALSE, 0);

  vbox2 = gtk_vbox_new (FALSE, 0);

  view_option_menu = gtk_option_menu_new ();
  view_menu = gtk_menu_new ();
  gtk_option_menu_set_menu(GTK_OPTION_MENU (view_option_menu) ,view_menu);

  view_menu_item = gtk_menu_item_new_with_label ("Icon view");
  gtk_widget_show (view_menu_item);
  gtk_menu_append (GTK_MENU (view_menu), view_menu_item);
  //gtk_signal_connect (GTK_OBJECT (view_menu_item), "activate", 
  //   (GtkSignalFunc) set_view, "icons");

  view_menu_item = gtk_menu_item_new_with_label ("List view");
  gtk_widget_show (view_menu_item);
  gtk_menu_append (GTK_MENU (view_menu), view_menu_item);
  //gtk_signal_connect (GTK_OBJECT (view_menu_item), "activate", 
  //   (GtkSignalFunc) set_view, "list");

  loading_label = gtk_label_new ("Loading...");
  info_label = gtk_label_new ("No Image");

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

  p = gpe_find_icon ("open");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Open"), 
			   _("Open file"), _("Open file"), pw, show_dirbrowser, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  p = gpe_find_icon ("left");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Previous"), 
			   _("Previous image"), _("Previous image"), pw, previous_image, NULL);

  p = gpe_find_icon ("right");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Next"), 
			   _("Next image"), _("Next image"), pw, next_image, NULL);

  p = gpe_find_icon ("cancel");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Close image"), 
			   _("Close image"), _("Close image"), pw, hide_image, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  p = gpe_find_icon ("slideshow");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Slideshow"), 
			   _("Slideshow"), _("Slideshow"), pw, show_new_slideshow, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar), view_option_menu,
			   _("Select View"), _("Select View"));

  p = gpe_find_icon ("fullscreen");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (tools_toolbar), _("Fullscreen"), 
			   _("Toggle fullscreen"), _("Toggle fullscreen"), pw, enable_fullscreen, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (tools_toolbar));

  p = gpe_find_icon ("zoom_in");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (tools_toolbar), _("Zoom In"), 
			   _("Zoom In"), _("Zoom In"), pw, image_zoom_in, NULL);

  p = gpe_find_icon ("zoom_out");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (tools_toolbar), _("Zoom Out"), 
			   _("Zoom Out"), _("Zoom Out"), pw, image_zoom_out, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (tools_toolbar));

  p = gpe_find_icon ("zoom_1");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (tools_toolbar), _("Zoom 1:1"), 
			   _("Zoom 1:1"), _("Zoom 1:1"), pw, image_zoom_1, NULL);

  p = gpe_find_icon ("zoom_fit");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (tools_toolbar), _("Zoom to Fit"), 
			   _("Zoom to Fit"), _("Zoom to Fit"), pw, image_zoom_fit, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (tools_toolbar));

  gtk_toolbar_append_widget (GTK_TOOLBAR (tools_toolbar), info_label,
			   _("Image Infomation"), _("Image Infomation"));

  gtk_toolbar_append_widget (GTK_TOOLBAR (loading_toolbar), loading_label,
			   _("Loading..."), _("Loading..."));

  gtk_toolbar_append_widget (GTK_TOOLBAR (loading_toolbar), loading_progress_bar,
			   _("Progress"), _("Progress"));

  p = gpe_find_icon ("stop");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (loading_toolbar), _("Stop Loading"), 
			   _("Stop Loading"), _("Stop Loading"), pw, NULL, NULL);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));
  gtk_box_pack_start (GTK_BOX (hbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), vbox2, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), loading_toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), tools_toolbar, FALSE, FALSE, 0);

  if (gpe_find_icon_pixmap ("icon", &pmap, &bmap))
    gdk_window_set_icon (window->window, NULL, pmap, bmap);

  gtk_widget_show (window);
  gtk_widget_show (vbox);
  gtk_widget_show (vbox2);
  gtk_widget_show (hbox);
  gtk_widget_show (toolbar);
  gtk_widget_show (info_label);
  gtk_widget_show (loading_label);
  gtk_widget_show (loading_progress_bar);
  gtk_widget_show (view_option_menu);
  gtk_widget_show (view_menu);

  if (argc > 1)
  {
    show_image (NULL, (gpointer) argv[1]);
  }

  gtk_main();
  return 0;
}
