/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
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

#include <libdm.h>

#include "todo.h"
#include "todo-sql.h"

static struct gpe_icon my_icons[] = {
  { "ok" },
  { "cancel" },
  { "save" },
  { "new" },
  { "hide" },
  { "clean" },
  { "properties" },
  { "delete"  },
  { "cancel" },
  { "exit" },
  { "question" },
  { "icon", PREFIX "/share/pixmaps/gpe-todo.png" },
  { "tick" },
  { NULL, NULL }
};

#define _(_x) gettext(_x)

guint window_x = 240, window_y = 320;

GtkWidget *the_notebook;
GtkWidget *window;

extern GtkWidget *top_level (GtkWidget *window);

static void
open_window (void)
{
  GtkWidget *top;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  libdm_mark_window (window);

  top = top_level (window);

  gtk_container_add (GTK_CONTAINER (window), top);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  gtk_window_set_default_size (GTK_WINDOW (window), 240, 320);
  gtk_window_set_title (GTK_WINDOW (window), _("To-do list"));
  gpe_set_window_icon (window, "icon");

  gtk_widget_show (window);
}

int
main(int argc, char *argv[])
{
  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  if (sql_start ())
    exit (1);

  libdm_init ();

  open_window ();
  
  gtk_main ();

  return 0;
}
