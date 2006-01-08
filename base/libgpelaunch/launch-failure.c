/*
 * GPE program launcher library
 *
 * Copyright (c) 2005 Phil Blundell
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

#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>

int
main (int argc, char *argv[])
{
  GtkWidget *w;
  const char *error;

  if (argc != 2)
    {
      fprintf (stderr, "usage: %s <error message>\n", argv[0]);
      exit (1);
    }

  error = argv[1];

  gtk_init (&argc, &argv);

  w = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CANCEL, error, NULL);
  
  gtk_dialog_run (GTK_DIALOG (w));

  exit (0);
}
