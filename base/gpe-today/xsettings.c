/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 * Adapted from gpe-calendar by Luis Oliveira <luis@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <gdk/gdkx.h>
#include <gpe/errorbox.h>
#include "xsettings-client.h"
#include "today.h"

#define KEY_BASE "GPE/"
#define MBG "MATCHBOX/Background"

static void notify_func(const char *name, XSettingsAction action,
                        XSettingsSetting *setting, void *cb_data)
{
    if (action != XSETTINGS_ACTION_NEW && action != XSETTINGS_ACTION_CHANGED)
        return;

    if (conf.bg == MBDESKTOP_BG && !strncmp(name, MBG, strlen(MBG))) {
        if (setting->type == XSETTINGS_TYPE_STRING) {
            if (strchr(setting->data.v_string, ':'))
                set_background(strrchr(setting->data.v_string, ':') + 1); 
            else
                set_background(setting->data.v_string);
        }
    } else if (!strncmp(name, KEY_BASE, strlen(KEY_BASE))) {
        const char *p = name + strlen(KEY_BASE);

        if (!strcmp(p, "Today/Background")
            && setting->type == XSETTINGS_TYPE_STRING) {
            
            set_background(setting->data.v_string);
        }
    }
}

static GdkFilterReturn xsettings_event_filter(GdkXEvent *xevp, GdkEvent *ev,
                                              gpointer p)
{
    if (xsettings_client_process_event(conf.xst_client, (XEvent *) xevp))
        return GDK_FILTER_REMOVE;
    
    return GDK_FILTER_CONTINUE;
}

static void watch_func(Window window, Bool is_start, long mask, void *cb_data)
{
    GdkWindow *gdkwin;
    
    gdkwin = gdk_window_lookup(window);
    
    if (is_start) {
        if (!gdkwin)
            gdkwin = gdk_window_foreign_new(window);
        else
            g_object_ref(gdkwin);
        
        gdk_window_add_filter(gdkwin, xsettings_event_filter, NULL);
    } else {
        assert(gdkwin);
        g_object_unref(gdkwin);
        gdk_window_remove_filter(gdkwin, xsettings_event_filter, NULL);
    }
}

gboolean start_xsettings(void)
{
    Display *dpy = GDK_DISPLAY();
    
    conf.xst_client = xsettings_client_new(dpy, DefaultScreen(dpy),
                                           notify_func, watch_func, NULL);
    
    if (conf.xst_client == NULL) {
        gpe_error_box (_("Cannot create XSettings client"));
        return FALSE;
    }
    
    return TRUE;
}
