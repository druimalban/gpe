/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
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

#ifndef PICTUREBUTTON_H
#define PICTUREBUTTON_H

enum
{
  GPE_BUTTON_TYPE_ICON,
  GPE_BUTTON_TYPE_LABEL,
  GPE_BUTTON_TYPE_BOTH
};

extern GtkWidget *gpe_picture_button (GtkStyle *style, 
				      gchar *text, gchar *icon);
extern GtkWidget *gpe_button_new_from_stock (const gchar *stock_id, int type);

#endif
