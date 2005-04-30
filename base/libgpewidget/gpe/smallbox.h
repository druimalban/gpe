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

#ifndef SMALLBOX_H
#define SMALLBOX_H

#include <gtk/gtk.h>

/**
 * box_desc:
 * 
 * Value pair to describe a query in a small box.
 *
 * The componet label describes the query text, value the initial value
 * of the text input field.
 */
struct box_desc
{
  gchar *label;
  gchar *value;
};

/**
 * box_desc2:
 * 
 * box_desc2 offers a more complex struct to decribe a query in a small box.
 *
 * The componet label describes the query text, value the initial value
 * of the text input field.
 *
 * Additionally it contains a list of suggestions to offer the user a set 
 * of predefined choices.
 */
struct box_desc2
{
  gchar *label;
  gchar *value;
  GList *suggestions;
};

extern gboolean smallbox_x (gchar *title, struct box_desc *d);
extern gboolean smallbox_x2 (gchar *title, struct box_desc2 *d);
extern gchar *smallbox (gchar *title, gchar *labeltext, gchar *dval);

#endif
