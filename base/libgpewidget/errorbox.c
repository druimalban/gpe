/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <gdk_imlib.h>
#include <libintl.h>
#include <errno.h>
#include <string.h>

#include "errorbox.h"

#define ERROR_ICON "/usr/share/pixmaps/error.png"

#define _(x) gettext(x)

static GdkPixmap *error_pix;
static GdkBitmap *error_pix_mask;
static gboolean error_pix_loaded;

void
gpe_error_box (char *text)
{
  GtkWidget *label, *ok, *dialog;
  GtkWidget *hbox;

  dialog = gtk_dialog_new ();
  label = gtk_label_new (text);
  ok = gtk_button_new_with_label (_("OK"));
  hbox = gtk_hbox_new (FALSE, 4);

  gtk_signal_connect_object (GTK_OBJECT (ok), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy), 
			     (gpointer)dialog);

  if (!error_pix_loaded)
    {
      error_pix_loaded = TRUE;
      gdk_imlib_load_file_to_pixmap (ERROR_ICON, &error_pix, 
				     &error_pix_mask);
    }

  if (error_pix)
    {
      GtkWidget *icon = gtk_pixmap_new (error_pix, error_pix_mask);
      gtk_box_pack_start (GTK_BOX (hbox), icon, TRUE, TRUE, 0);
    }

  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), ok);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);

  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  gtk_widget_show_all (dialog);

  gtk_main ();
}

void
gpe_perror_box(char *text)
{
  char *p = strerror (errno);
  char *buf = g_malloc (strlen (p) + strlen (text) + 3);
  strcpy (buf, text);
  strcat (buf, ": ");
  strcat (buf, p);
  gpe_error_box (buf);
  g_free (buf);
}
