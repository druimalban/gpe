/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
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
