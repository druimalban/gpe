/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <libintl.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "errorbox.h"
#include "render.h"
#include "picturebutton.h"
#include "pixmaps.h"

static struct gpe_icon my_icons[] = {
  { "ok", "ok" },
  { "error", "error" },
  { NULL, NULL }
};

static gboolean currently_handling_error = FALSE;

#define _(x) dgettext(PACKAGE, x)

void
gpe_error_box (char *text)
{
  GtkWidget *label, *ok, *dialog, *icon;
  GtkWidget *hbox;
  GdkPixbuf *p;

  if (currently_handling_error)
    {
      fprintf (stderr, "%s\n", text);
      return;
    }

  currently_handling_error = TRUE;

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  dialog = gtk_dialog_new ();
  label = gtk_label_new (text);
  ok = gpe_picture_button (dialog->style, _("OK"), "ok");
  hbox = gtk_hbox_new (FALSE, 4);

  gtk_signal_connect_object (GTK_OBJECT (ok), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy), 
			     (gpointer)dialog);

  gtk_widget_realize (dialog);

  p = gpe_find_icon ("error");
  icon = gpe_render_icon (GTK_DIALOG (dialog)->vbox->style, p);
  gtk_box_pack_start (GTK_BOX (hbox), icon, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 4);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), ok);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);

  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  gtk_widget_show_all (dialog);

  currently_handling_error = FALSE;

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

void
gpe_error_box_fmt(const char *format, ...)
{
  va_list ap;
  char *str;

  va_start (ap, format);
  vasprintf (&str, format, ap);
  va_end (ap);
  gpe_error_box (str);
  g_free (str);
}
