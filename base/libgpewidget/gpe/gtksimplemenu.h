/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
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

GtkType		gtk_simple_menu_get_type	(void);
GtkWidget      *gtk_simple_menu_new		(void);

guint		gtk_simple_menu_get_result	(GtkSimpleMenu *sel);
void            gtk_simple_menu_append_item	(GtkSimpleMenu *sel, const gchar *items);
void		gtk_simple_menu_flush		(GtkSimpleMenu *sel);

#endif
