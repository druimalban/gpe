/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>

GtkWidget*
gtk_dialog_new (void)
{
  GtkWidget *w = GTK_WIDGET (gtk_type_new (GTK_TYPE_DIALOG));
  GTK_WINDOW (w)->type = GTK_WINDOW_DIALOG;
  return w;
}
