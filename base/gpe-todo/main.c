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

#include <gtk/gtk.h>
#include <gdk_imlib.h>

#include "todo.h"
#include "todo-sql.h"

#define _(_x) gettext(_x)

guint window_x = 240, window_y = 320;

GtkWidget *the_notebook;
GtkWidget *window;

extern GtkWidget *top_level (void);

static void
open_window (void)
{
  GtkWidget *listbook = top_level();
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  the_notebook = listbook;
  gtk_container_add (GTK_CONTAINER (window), listbook);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  gtk_widget_set_usize (window, 240, 320);
  gtk_widget_show_all (window);
}

int
main(int argc, char *argv[])
{
  gtk_set_locale ();
  gtk_init (&argc, &argv);
  gdk_imlib_init ();

  if (sql_start ())
    exit (1);

  open_window ();
  
  gtk_main ();

  return 0;
}
