/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
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

#ifndef GTK_SIMPLE_MENU_H
#define GTK_SIMPLE_MENU_H

#include <gtk/gtk.h>

#define GTK_TYPE_SIMPLE_MENU                  (gtk_simple_menu_get_type ())
#define GTK_SIMPLE_MENU(obj)                  (GTK_CHECK_CAST ((obj), GTK_TYPE_SIMPLE_MENU, GtkSimpleMenu))
#define GTK_SIMPLE_MENU_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_SIMPLE_MENU, GtkSimpleMenuClass))
#define GTK_IS_SIMPLE_MENU(obj)               (GTK_CHECK_TYPE ((obj), GTK_TYPE_SIMPLE_MENU))
#define GTK_IS_SIMPLE_MENU_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_SIMPLE_MENU))

typedef struct _GtkSimpleMenu	   GtkSimpleMenu;
typedef struct _GtkSimpleMenuClass GtkSimpleMenuClass;

GtkType	gtk_simple_menu_get_type (void);
GtkWidget *gtk_simple_menu_new (void);

void gtk_simple_menu_append_item(GtkSimpleMenu *sel, const gchar *item);
void gtk_simple_menu_flush (GtkSimpleMenu *sel);

#endif
