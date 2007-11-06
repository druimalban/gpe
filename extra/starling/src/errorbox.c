/*
 * This file is part of Starling
 *
 * Copyright (C) 2006 Alberto García Hierro
 *      <skyhusker@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdarg.h>
#include <glib.h>

#ifdef ENABLE_GPE
#   include <gpe/errorbox.h>
#else
#   include <gtk/gtkmessagedialog.h>
#   include <gtk/gtkdialog.h>
#endif

#include "errorbox.h"

void
starling_error_box (const gchar *text)
{

#ifdef ENABLE_GPE

    gpe_error_box (text);

#else

    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
                                GTK_MESSAGE_ERROR,
                                GTK_BUTTONS_OK, text);

    gtk_dialog_run (GTK_DIALOG (dialog));
    
    gtk_widget_destroy (dialog);

#endif

}

void
starling_error_box_fmt (const gchar *fmt, ...)
{
    va_list ap;
    gchar *text;

    va_start (ap, fmt);
    vasprintf (&text, fmt, ap);
    va_end (ap);

    starling_error_box (text);

    g_free (text);
}

