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
#include "pixmaps.h"
#include "init.h"

#define PIXMAPS_DIR "/usr/share/gpe/pixmaps"
#define MY_PIXMAPS_DIR "/usr/share/gpe-todo/pixmaps"

struct pix my_pix[] = {
  { "new", MY_PIXMAPS_DIR "/new.png" },
  { "config", MY_PIXMAPS_DIR "/preferences.png" },
  { "delete", MY_PIXMAPS_DIR "/trash.png" },
  { NULL, NULL }
};

#define _(_x) gettext(_x)

guint window_x = 240, window_y = 320;

GtkWidget *the_notebook;
GtkWidget *window;

extern GtkWidget *top_level (void);

static void
open_window (void)
{
  GtkWidget *top = top_level();
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_container_add (GTK_CONTAINER (window), top);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  gtk_widget_set_usize (window, 240, 320);
  gtk_widget_show (window);
}

int
main(int argc, char *argv[])
{
  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (load_pixmaps (my_pix) == FALSE)
    exit (1);

  if (sql_start ())
    exit (1);

  open_window ();
  
  gtk_main ();

  return 0;
}
