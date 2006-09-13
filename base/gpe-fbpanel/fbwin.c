/*
 * fb-background-monitor.c:
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
 *      Mark McLoughlin <mark@skynet.ie>
 */

#include <glib.h>
#include <glib-object.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "win.h"
#include "misc.h"

//#define DEBUG
#include "dbg.h"



struct _FbWinClass {
    GObjectClass   parent_class;
 
};

struct _FbWin {
    GObject    parent_instance;

    int desktop_no;
    Window xwin;
    gchar *name;
    int x, y, w, h;
    GdkPixbuf *pixbuf;
    
};

static void fb_win_class_init (FbWinClass *klass);
static void fb_win_init (FbWin *monitor);
static void fb_win_finalize (GObject *object);

void fb_win_update_name(FbWin *win);

GType
fb_win_get_type (void)
{
    static GType object_type = 0;

    if (!object_type) {
        static const GTypeInfo object_info = {
            sizeof (FbWinClass),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) fb_win_class_init,
            NULL,           /* class_finalize */
            NULL,           /* class_data */
            sizeof (FbWin),
            0,              /* n_preallocs */
            (GInstanceInitFunc) fb_win_init,
        };

        object_type = g_type_register_static (
            G_TYPE_OBJECT, "FbWin", &object_info, 0);
    }

    return object_type;
}



static void
fb_win_class_init (FbWinClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = fb_win_finalize;
    
}

static void
fb_win_init (FbWin *win)
{
    win->x = win->y = win->w = win->h = 1;
    win->win = None;
    win->name = NULL;
    win->desktop_no = 0;
}


FbWin *
fb_win_new(Window xwin)
{
    FbWin *win = g_object_new (FB_TYPE_WIN, NULL);
    win->win = xwin;
}

static void
fb_win_finalize (GObject *object)
{
    FbWin *win;

    win = FB_WIN (object);
    if (win->name)
        g_free(win->name);
    //XFreeGC(win->dpy, win->gc);
}


void
fb_win_update_name(FbWin *win)
{
    if (win->name)
        g_free(win->name);
     
    win->name = get_utf8_property(win->xwin,  a_NET_WM_NAME);
    if (win->name)
        DBG2("a_NET_WM_NAME=%s\n", win->name);
    if (!win->name) {
        win->name = get_textproperty(win->xwin,  XA_WM_NAME);
        if (win->name)
            DBG2("XA_WM_NAME=%s\n", win->name);
    }
    RET();

}
