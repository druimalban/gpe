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

#define WINDOW_NAME "GPE Gallery"
#define _(_x) gettext (_x)
GtkWidget *window;
GtkWidget *dirbrowser_window;
GtkWidget *view_widget;

// 0 = Detailed list
// 1 = Thumbs
gint current_view = 0;
gint loading_directory = 0;

struct gpe_icon my_icons[] = {
  { "open", "open" },
  { "left", "left" },
  { "right", "right" },
  { "slideshow", "gpe-gallery/slideshow" },
  { "fullscreen", "gpe-gallery/fullscreen" },
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
add_directory (gchar *directory)
{
  struct dirent *d;
  DIR *dir;

  loading_directory = 1;
  printf ("Selected directory: %s\n", directory);

  dir = opendir (current_directory);
  if (dir)
  {
    if (view_widget)
      gtk_widget_destroy (view_widget);

    if (current_view == 0) // Detailed list
    {
      view_widget = gtk_vbox_new (FALSE, 0);
    }

    while (d = readdir (dir), d != NULL)
    {
      if (loading_directory == 0)
        break;

      if (d->d_name[0] != '.')
      {
        struct stat s;
        filename = g_strdup (d->d_name);

        if (stat (buf, &s) == 0)
        {
        }
      }
    }
  }
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
  GtkWidget *vbox, *hbox, *toolbar, *scroll;
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

#if GTK_MAJOR_VERSION < 2
  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_SPACE_LINE);
#else
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
#endif

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  p = gpe_find_icon ("open");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Open"), 
			   _("Open file"), _("Open file"), pw, show_dirbrowser, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  p = gpe_find_icon ("left");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Previous"), 
			   _("Previous image"), _("Previous image"), pw, NULL, NULL);

  p = gpe_find_icon ("right");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Next"), 
			   _("Next image"), _("Next image"), pw, NULL, NULL);

  p = gpe_find_icon ("stop");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Close image"), 
			   _("Close image"), _("Close image"), pw, NULL, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  p = gpe_find_icon ("fullscreen");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Fullscreen"), 
			   _("Toggle fullscreen"), _("Toggle fullscreen"), pw, NULL, NULL);

  p = gpe_find_icon ("slideshow");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Slideshow"), 
			   _("Slideshow"), _("Slideshow"), pw, NULL, NULL);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));
  gtk_box_pack_start (GTK_BOX (hbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), view_option_menu, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  //gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET (text_area));
  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);

  if (gpe_find_icon_pixmap ("icon", &pmap, &bmap))
    gdk_window_set_icon (window->window, NULL, pmap, bmap);

  gtk_widget_show (window);
  gtk_widget_show (vbox);
  gtk_widget_show (hbox);
  gtk_widget_show (toolbar);
  gtk_widget_show (view_option_menu);
  gtk_widget_show (view_menu);
  gtk_widget_show (scroll);

  gtk_main();
  return 0;
}