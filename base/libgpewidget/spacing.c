/*
 * Copyright (C) 2002, 2003 Colin Marquardt <ipaq@marquardt-home.de>
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

#include "spacing.h"
#include <gdk/gdk.h>

guint GPE_GNOME_SCALING = 2;

/* determine scaling by querying display size */
void 
init_spacing(void) {
	if (gdk_screen_width() > SCALING_SIZE)
		GPE_GNOME_SCALING = 1;
}

/* the spacing for categories in a dialog: */
guint
gpe_get_catspacing () {
  return 18/GPE_GNOME_SCALING;
}

/* the indent for categories in a dialog: */
gchar *
gpe_get_catindent () {
  return "  "; /* Gnome 2 uses four spaces */
}

/* the border for e.g. a GtkDialog: */
guint
gpe_get_border () {
  return 12/GPE_GNOME_SCALING;
}

/* the spacing in boxes: */
guint
gpe_get_boxspacing () {
  return 6/GPE_GNOME_SCALING;
}
