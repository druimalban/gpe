/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef SMALLBOX_H
#define SMALLBOX_H

#include <gtk/gtk.h>

struct box_desc
{
  gchar *label;
  gchar *value;
};

extern gboolean smallbox_x (gchar *title, struct box_desc *d);
extern gchar *smallbox (gchar *title, gchar *labeltext, gchar *dval);

#endif
