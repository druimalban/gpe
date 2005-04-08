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

#ifndef SPACING_H
#define SPACING_H

#include <glib.h>

/*
 * How much to scale down the spacings defined in the Gnome 2 HIG
 *   (http://developer.gnome.org/projects/gup/hig/1.0/layout.html#layout-window)
 * for GPE:
 */
#define SCALING_SIZE	400
extern guint GPE_GNOME_SCALING;

/* See also
 *  http://mail.gnome.org/archives/gtk-devel-list/2003-February/msg00009.html
 */
 
/* determine scaling by querying display size */
void init_spacing (void);

/* the spacing for categories in a dialog: */
guint gpe_get_catspacing (void);

/* the indent for categories in a dialog: */
gchar *gpe_get_catindent (void);

/* the border for e.g. a GtkDialog: */
guint gpe_get_border (void);

/* the spacing in boxes: */
guint gpe_get_boxspacing (void);

#endif
