/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <libintl.h>

#include <gdk/gdkx.h>

#include <gpe/errorbox.h>

#include "xsettings-client.h"

#include "globals.h"

static XSettingsClient *client;
static void (*push_new_changes) (void);

#define KEY_BASE "GPE/"

static void
notify_func (const char *name, XSettingsAction action,
	     XSettingsSetting *setting, void *cb_data)
{
  if ((action == XSETTINGS_ACTION_NEW
       || action == XSETTINGS_ACTION_CHANGED)
      && strncmp (name, KEY_BASE, strlen (KEY_BASE)) == 0)
    {
      const char *p = name + strlen (KEY_BASE);
      if (!strcasecmp (p, "week-starts-monday")
	  || !strcasecmp (p, "Calendar/week-starts-monday"))
	{
	  if (setting->type == XSETTINGS_TYPE_INT)
	    {
	      week_starts_sunday = setting->data.v_int ? FALSE : TRUE;
	      if (week_starts_sunday)
		week_offset = 1;
	      else
		week_offset = 0;
	      if (push_new_changes)
		push_new_changes ();
	    }
	}
      else if (!strcasecmp (p, "Calendar/day-view-combined-times"))
	{
	  if (setting->type == XSETTINGS_TYPE_INT)
	    {
	      day_view_combined_times = setting->data.v_int ? TRUE : FALSE;
	      if (push_new_changes)
		push_new_changes ();
	    }
	}
    }
}

static GdkFilterReturn
xsettings_event_filter (GdkXEvent *xevp, GdkEvent *ev, gpointer p)
{
  if (xsettings_client_process_event (client, (XEvent *)xevp))
    return GDK_FILTER_REMOVE;

  return GDK_FILTER_CONTINUE;
}

static void 
watch_func (Window window, Bool is_start, long mask,
	    void *cb_data)
{
  GdkWindow *gdkwin;
  
  gdkwin = gdk_window_lookup (window);

  if (is_start)
    {
      if (!gdkwin)
	gdkwin = gdk_window_foreign_new (window);
      else
	g_object_ref (gdkwin);

      gdk_window_add_filter (gdkwin, xsettings_event_filter, NULL);
    }
  else
    {
      assert (gdkwin);
      g_object_unref (gdkwin);
      gdk_window_remove_filter (gdkwin, xsettings_event_filter, NULL);
    }
}

gboolean
gpe_calendar_start_xsettings (void (*push_changes) (void))
{
  Display *dpy = GDK_DISPLAY ();

  client = xsettings_client_new (dpy, DefaultScreen (dpy), notify_func, watch_func, NULL);
  if (client == NULL)
    {
      gpe_error_box (_("Cannot create XSettings client"));
      return FALSE;
    }

  push_new_changes = push_changes;

  return TRUE;
}
