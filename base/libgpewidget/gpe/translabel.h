/*
 * Copyright (C) 2001, 2002, 2003 Philip Blundell <philb@gnu.org>
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

#ifndef GPE_TRANSLABEL_H
#define GPE_TRANSLABEL_H

extern void gtk_widget_add_translation_hook (GtkWidget *w, void (*func)(GtkWidget *, void *), gpointer data);
extern GtkWidget *gtk_label_new_with_translation (gchar *domain, gchar *string);

#endif
