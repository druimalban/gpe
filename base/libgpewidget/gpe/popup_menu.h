/*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
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

#ifndef POPUP_MENU_H
#define POPUP_MENU_H

extern GtkWidget *popup_menu_button_new (GtkWidget *child, GtkWidget *(construct_func)(), void *(callback_func)());
extern GtkWidget *popup_menu_button_new_from_stock (const gchar *stock_id, GtkWidget *(construct_func)(), void *(callback_func)());
extern void toggle_popup_menu (GtkWidget *parent_button, GtkWidget *(construct_func)());
extern void popup_menu_close (GtkWidget *parent_button);

extern GtkWidget *popup_menu_button_new_type_font (void *(callback_func)());

#endif
