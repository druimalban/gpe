/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

extern gboolean gpe_get_client_window_list (Display *dpy, Window **list, guint *nr);
extern gchar *gpe_get_window_name (Display *dpy, Window w);
extern GdkPixbuf *gpe_get_window_icon (Display *dpy, Window w);
