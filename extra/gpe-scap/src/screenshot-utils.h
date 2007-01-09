/* This file is a copy of gnome-screenshot/screenshot-utils.h from
 * gnome-utils-2.10.0 which does not have an individual copyright
 * header.
 *
 * According to the copyright header from
 * gnome-screenshot/gnome-panel-screenshot.c gnome-screenshot is
 * licensed under the terms of the GNU GPL version 2 or later.
 *
 * The file COPYING distributed with gnome-utils-2.10.0 contains
 * the same license.
 *
 */

#ifndef __SCREENSHOT_UTILS_H__
#define __SCREENSHOT_UTILS_H__

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

gboolean   screenshot_grab_lock           (void);
void       screenshot_release_lock        (void);
gchar     *screenshot_get_window_title    (Window   w);
Window     screenshot_find_current_window (gboolean include_decoration);
GdkPixbuf *screenshot_get_pixbuf          (Window   w);

#endif /* __SCREENSHOT_UTILS_H__ */
