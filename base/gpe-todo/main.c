/*
 * Copyright (C) 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>
#include <locale.h>

#include <gtk/gtk.h>
#include <gpe/pixmaps.h>
#include <gpe/init.h>
#include <gpe/pim-categories.h>

#include <libdisplaymigration/displaymigration.h>

#include "todo.h"

#define MY_PIXMAPS_DIR PREFIX "/share/gpe-todo"

static struct gpe_icon my_icons[] = {
  { "hide" },
  { "clean" },
  { "icon", PREFIX "/share/pixmaps/gpe-todo.png" },
  { "tick-box", MY_PIXMAPS_DIR "/tick-box.png" },
  { "notick-box", MY_PIXMAPS_DIR "/notick-box.png" },
  { "bar-box", MY_PIXMAPS_DIR "/bar-box.png" },
  { "dot-box", MY_PIXMAPS_DIR "/dot-box.png" },
  { NULL, NULL }
};

#define _(_x) gettext(_x)

GtkWidget *the_notebook;
GtkWidget *window;

extern GtkWidget *top_level (GtkWidget *window);

static void
open_window (void)
{
  GtkWidget *top;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  displaymigration_mark_window (window);

  top = top_level (window);

  gtk_container_add (GTK_CONTAINER (window), top);

  g_signal_connect (G_OBJECT (window), "delete-event",
		    G_CALLBACK (gtk_main_quit), NULL);

  gtk_window_set_default_size (GTK_WINDOW (window), 240, 320);
  gtk_window_set_title (GTK_WINDOW (window), _("To-do list"));
  gpe_set_window_icon (window, "icon");

  gtk_widget_show_all (window);
}

int
main (int argc, char *argv[])
{
  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  if (todo_db_start ())
    exit (1);

  if (gpe_pim_categories_init () == FALSE)
    exit (1);  

  displaymigration_init ();

  open_window ();
  
  gtk_main ();

  return 0;
}
