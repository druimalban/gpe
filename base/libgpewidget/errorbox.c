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

static volatile gboolean currently_handling_error = FALSE;

#define _(x) dgettext(PACKAGE, x)

static void
do_gpe_error_box (const char *text, gboolean block)
{
  GtkWidget *label, *ok, *dialog, *icon;
  GtkWidget *hbox;
  GdkPixbuf *p;
  GtkWidget *fakewindow;

  fprintf (stderr, "GPE-ERROR: %s\n", text);

  if (currently_handling_error)
    return;

  currently_handling_error = TRUE;
  fakewindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (fakewindow);

  dialog = gtk_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW(dialog), GTK_WINDOW(fakewindow));
  gtk_widget_realize (dialog);

  gtk_window_set_title (GTK_WINDOW(dialog), _("Error"));

  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  label = gtk_label_new (text);
  hbox = gtk_hbox_new (FALSE, 4);

#if GTK_MAJOR_VERSION >= 2
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
#endif

  gtk_widget_realize (dialog);

  ok = gpe_picture_button (dialog->style, _("OK"), "ok");
  gtk_signal_connect_object (GTK_OBJECT (ok), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy), 
			     (gpointer)dialog);

  p = gpe_find_icon ("error");
  icon = gpe_render_icon (GTK_DIALOG (dialog)->vbox->style, p);
  gtk_box_pack_start (GTK_BOX (hbox), icon, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 4);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), ok);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);

  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  if (block)
    {
      gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
    }
      
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
                    GTK_SIGNAL_FUNC (gtk_widget_destroy),(gpointer)fakewindow);

  gtk_widget_show_all (dialog);

  if (block)
    gtk_main ();

  currently_handling_error = FALSE;
}

void
gpe_error_box (const char *text)
{
  do_gpe_error_box (text, TRUE);
}

void
gpe_error_box_nonblocking (const char *text)
{
  do_gpe_error_box (text, FALSE);
}

void
gpe_perror_box(const char *text)
{
  char *p = strerror (errno);
  char *buf = g_malloc (strlen (p) + strlen (text) + 3);
  strcpy (buf, text);
  strcat (buf, ":\n");
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
