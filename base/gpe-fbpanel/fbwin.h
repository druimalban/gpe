/*
 * fb-background-monitor.h:
 *
 * Copyright (C) 2001, 2002 Ian McKellar <yakk@yakk.net>
 *                     2002 Sun Microsystems, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors:
 *      Ian McKellar <yakk@yakk.net>
 *	Mark McLoughlin <mark@skynet.ie>
 */

#ifndef __FB_WIN_H__
#define __FB_WIN_H__

/* FIXME: this needs to be made multiscreen aware
 *        panel_bg_get should take
 *        a GdkScreen argument.
 */

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#define FB_TYPE_WIN         (fb_win_get_type ())
#define FB_WIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o),      \
					       FB_TYPE_WIN,        \
					       FbWin))
#define FB_WIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k),         \
					       FB_TYPE_WIN,        \
					       FbWinClass))
#define FB_IS_WIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o),      \
					       FB_TYPE_WIN))
#define FB_IS_WIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),         \
					       FB_TYPE_WIN))
#define FB_WIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),       \
					       FB_TYPE_WIN,        \
					       FbWinClass))

typedef struct _FbWinClass FbWinClass;
typedef struct _FbWin      FbWin;
enum {
    FBWIN_NAME,
    FBWIN_GEOM,
    FBWIN_ICON,
    FBWIN_STATE,
    FBWIN_LAST_SIGNAL
};

GType fb_win_get_type       (void);
FbWin *fb_win_new(void);
void fb_win_trigger(FbWin *win, int signal);


#endif /* __FB_WIN_H__ */
