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

#ifndef __FB_BG_H__
#define __FB_BG_H__

/* FIXME: this needs to be made multiscreen aware
 *        panel_bg_get should take
 *        a GdkScreen argument.
 */

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#define FB_TYPE_BG         (fb_bg_get_type ())
#define FB_BG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o),      \
					       FB_TYPE_BG,        \
					       FbBg))
#define FB_BG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k),         \
					       FB_TYPE_BG,        \
					       FbBgClass))
#define FB_IS_BG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o),      \
					       FB_TYPE_BG))
#define FB_IS_BG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),         \
					       FB_TYPE_BG))
#define FB_BG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),       \
					       FB_TYPE_BG,        \
					       FbBgClass))

typedef struct _FbBgClass FbBgClass;
typedef struct _FbBg      FbBg;

GType fb_bg_get_type       (void);
FbBg *fb_bg_new(void);
void fb_bg_composite(GdkDrawable *base, GdkGC *gc, guint32 tintcolor, gint alpha);
GdkPixmap *fb_bg_get_xroot_pix_for_win(FbBg *bg, GtkWidget *widget);
void fb_bg_notify_changed_bg(FbBg *bg);
FbBg *fb_bg_get_for_display(void);
#endif /* __FB_BG_H__ */
