/*
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <libintl.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "errorbox.h"
#include "picturebutton.h"
#include "pixmaps.h"

static volatile gboolean currently_handling_error = FALSE;

#define _(x) dgettext(PACKAGE, x)

static void
do_gpe_error_box (const char *text, gboolean block)
{
  GtkWidget *w;

  fprintf (stderr, _("GPE-ERROR: %s\n"), text);

  if (currently_handling_error)
    return;

  currently_handling_error = TRUE;
  
  w = gtk_message_dialog_new (NULL, 
			      block ? GTK_DIALOG_MODAL : 0,
			      GTK_MESSAGE_ERROR,
			      GTK_BUTTONS_OK,
			      text);

  if (block)
    {
      gtk_dialog_run (GTK_DIALOG (w));
      gtk_widget_destroy (w);
    }
  else
    {
      g_signal_connect_swapped (G_OBJECT (w), "response",
				G_CALLBACK (gtk_widget_destroy),
				G_OBJECT (w));
      gtk_widget_show (w);
    }
  
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

static void
do_gpe_perror_box(const char *text, gboolean block)
{
  char *p = strerror (errno);
  char *buf = g_malloc (strlen (p) + strlen (text) + 3);
  strcpy (buf, text);
  strcat (buf, ":\n");
  strcat (buf, p);
  do_gpe_error_box (buf, block);
  g_free (buf);
}

void
gpe_perror_box (const char *text)
{
  do_gpe_perror_box (text, TRUE);
}

void
gpe_perror_box_nonblocking (const char *text)
{
  do_gpe_perror_box (text, FALSE);
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
