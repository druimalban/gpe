/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

/*
 * $Id$
 */

#include <gtk/gtk.h>
#include <libintl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <fcntl.h>

#include "export.h"
#include "main.h"
#include "support.h"
#include "db.h"

#include <gpe/vcard.h>
#include <gpe/errorbox.h>

static gchar *
export_to_vcard (guint uid)
{
  MIMEDirVCard *vcard;
  gchar *str;
  struct person *p;

  p = db_get_by_uid (uid);
  vcard = vcard_from_tags (p->data);
  discard_person (p);

  str = mimedir_vcard_write_to_string (vcard);
  g_object_unref (vcard);

  return str;
}

void
menu_do_send_bluetooth (void)
{
}

static void
select_file_done (GtkWidget *w, GtkWidget *filesel)
{
  guint uid;
  const gchar *filename;
  gchar *card;
  int fd;

  uid = (guint)g_object_get_data (G_OBJECT (filesel), "uid");

  card = export_to_vcard (uid);

  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel));

  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0)
    goto error;

  if (write (fd, card, strlen (card)) < strlen (card))
    goto error;
  
  if (close (fd))
    goto error;

  g_free (card);
  gtk_widget_destroy (filesel);
  return;

 error:
  g_free (card);
  gpe_perror_box (filename);
  gtk_widget_destroy (filesel);
}

void
menu_do_save (void)
{
  GtkWidget *filesel;

  filesel = gtk_file_selection_new (_("Save as..."));

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel), "GPE.vcf");

  g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button), 
			    "clicked", G_CALLBACK (gtk_widget_destroy), filesel);
  
  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button), 
		    "clicked", G_CALLBACK (select_file_done), filesel);

  g_object_set_data (G_OBJECT (filesel), "uid", (gpointer)menu_uid);
  
  gtk_widget_show_all (filesel);
}

