/* gpe-sketchbook -- a sketches notebook program for PDA
 * Copyright (C) 2002 Luc Pionchon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <gtk/gtk.h>

typedef struct _preferences {
  gboolean joypad_scroll;
  gboolean grow_on_scroll;
  //int start_with;
} Preferences;

void prefs_reset_defaults();

GtkWidget * preferences_gui(GtkWidget * window);

#endif
