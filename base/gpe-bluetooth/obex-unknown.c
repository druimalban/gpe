/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <libintl.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gpe/errorbox.h>
#include <gpe/question.h>

#include "obexserver.h"

#define _(x)  (x)

static void
select_file_done (GtkWidget *w, GtkWidget *filesel)
{
  gchar *data;
  size_t len;
  const gchar *filename;
  int fd;

  data = g_object_get_data (G_OBJECT (filesel), "data");
  len = (size_t)g_object_get_data (G_OBJECT (filesel), "data_length");

  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel));

  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd >= 0)
    {
      if (write (fd, data, len) < len)
	gpe_perror_box (filename);

      if (close (fd))
	gpe_perror_box (filename);
    }
  else
    gpe_perror_box (filename);
  
  gtk_widget_destroy (filesel);
}

void
import_unknown (const char *name, const gchar *data, size_t len)
{
  gchar *query;

  query = g_strdup_printf (_("Received a file '%s' (%d bytes).  Save it?"), name ? name : _("<no name>"), len);
  
  if (gpe_question_ask (query, NULL, "bt-logo", "!gtk-cancel", NULL, "!gtk-ok", NULL, NULL))
    {
      GtkWidget *filesel;
      const gchar *home;
      gchar *datap, *default_name;

      datap = g_malloc (len);
      memcpy (datap, data, len);

      home = g_get_home_dir ();

      if (name == NULL)
	name = "OBEX.dat";

      default_name = g_strdup_printf ("%s/%s", home, name);

      filesel = gtk_file_selection_new (_("Save as..."));

      gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel), default_name);

      g_free (default_name);

      g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button), 
				"clicked", G_CALLBACK (gtk_widget_destroy), filesel);

      g_signal_connect_swapped (G_OBJECT (filesel), "destroy", G_CALLBACK (g_free), datap);

      g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button), 
			"clicked", G_CALLBACK (select_file_done), filesel);

      g_object_set_data (G_OBJECT (filesel), "data", datap);
      g_object_set_data (G_OBJECT (filesel), "data_length", (gpointer)len);

      gtk_widget_show (filesel);
    }
}
