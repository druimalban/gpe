/*
 * Copyright (C) 2002 Colin Marquardt <ipaq@marquardt-home.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include "spacing.h"

/*
 * How much to scale down the spacings defined in the Gnome 2 HIG
 *   (http://developer.gnome.org/projects/gup/hig/1.0/layout.html#layout-window)
 * for GPE:
 */
#define GPE_GNOME_SCALING 2


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
