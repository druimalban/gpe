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
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gtk/gtk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include <openobex/obex.h>

#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>

#include "obexclient.h"
#include "obex-glib.h"
#include "main.h"
#include "sdp.h"

#define _(x) gettext(x)

struct file_push_context
{
  gchar *filename;
  gchar *data;
};

static void
file_push_done (gboolean result, gpointer data)
{
  struct file_push_context *c = data;

  g_free (c->filename);
  g_free (c->data);
  g_free (c);
}

static void
select_file_done (GtkWidget *w, GtkWidget *filesel)
{
  gchar *data;
  size_t len;
  gchar *filename;
  int fd;
  struct stat sb;
  struct file_push_context *c;

  filename = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel)));
  fd = open (filename, O_RDONLY);
  if (fd < 0)
    {
      gpe_perror_box (filename);
      goto error;
    }

  if (fstat (fd, &sb))
    {
      gpe_perror_box ("stat");
      close (fd);
      goto error;
    }
  
  len = sb.st_size;
  data = g_malloc (len);
  if (read (fd, data, len) < len)
    {
      gpe_perror_box ("read");
      g_free (data);
      close (fd);
      goto error;
    }

  close (fd);

  c = g_malloc (sizeof (*c));
  c->filename = filename;
  c->data = data;

  if (obex_object_push (filename, NULL, data, len, G_CALLBACK (file_push_done), c) == FALSE)
    {
      g_free (data);
      g_free (filename);
      g_free (c);
    }
      
  gtk_widget_destroy (filesel);
  return;

 error:
  g_free (filename);
  gtk_widget_destroy (filesel);
}

void
send_file_dialog (void)
{
  GtkWidget *filesel;

  filesel = gtk_file_selection_new (_("Select file"));

  g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button), 
			    "clicked", G_CALLBACK (gtk_widget_destroy), filesel);

  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button), 
		    "clicked", G_CALLBACK (select_file_done), filesel);

  gtk_widget_show_all (filesel);
}
