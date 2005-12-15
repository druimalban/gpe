/*
 * Copyright (C) 2005 Philippe De Swert <philippedeswert@scarlet.be>
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
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "gpedialog.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(x) dgettext(PACKAGE, x)
#else
#define _(x) (x)
#endif

/*
 * do_gpe_info_dialog:
 * @text: Text to display in the dialog
 * @block: Boolean that makes the dialog blocking or non-blocking
 *
 *  Displays an informational dialog to the user with the text 
 *  atring that was given as an argument. Pressing ok or on the
 *  close button of the window will destroy it.
 *
 *  Returns: nothing (void) 
 */

static void
do_gpe_info_dialog (const char *text, gboolean block)
{
  GtkWidget *w;

  w = gtk_message_dialog_new (NULL,
			      block ? GTK_DIALOG_MODAL : 0,
			      GTK_MESSAGE_INFO, GTK_BUTTONS_OK, text);

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

}

void
gpe_info_dialog (const char *text)
{
  do_gpe_info_dialog (text, TRUE);
}

void
gpe_info_dialog_nonblocking (const char *text)
{
  do_gpe_info_dialog (text, FALSE);
}
