/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
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

#ifndef GPE_POPUP_H
#define GPE_POPUP_H

#include <X11/Xlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int gpe_get_win_position (Display *dpy, Window win, int *x, int *y);
void gpe_popup_menu_position (GtkMenu *menu, gint *x, gint *y, gboolean *push, gpointer p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
