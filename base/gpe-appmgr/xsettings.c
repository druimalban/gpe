/*
 * Copyright (C) 2001, 2002, 2004 Philip Blundell <philb@gnu.org>
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

#include "xsettings-client.h"
#include "main.h"
#include "cfg.h"

static XSettingsClient *client;

#define KEY_BASE "GPE/appmgr/"

extern void row_view_set_bg_colour (guint c);
extern void row_view_set_background (gchar *c);
extern void row_view_refresh_background (void);

enum image_type
  {
    CENTERED,
    TILED,
    SCALED
  };

static void
notify_func (const char       *name,
	     XSettingsAction   action,
	     XSettingsSetting *setting,
	     void             *cb_data)
{
  if (strncmp (name, KEY_BASE, strlen (KEY_BASE)) == 0)
    {
      char *p = (char*)name + strlen (KEY_BASE);

      if (!strcmp (p, "bgcol") 
	  && setting->type == XSETTINGS_TYPE_COLOR
	  && flag_rows)
	{
	  guint c, r, g, b, a;
	  r = setting->data.v_color.red >> 8;
	  g = setting->data.v_color.green >> 8;
	  b = setting->data.v_color.blue >> 8;
	  a = setting->data.v_color.alpha >> 8;
	  c = (a << 24) | (r << 16) | (g << 8) | b;
	  row_view_set_bg_colour (c);
	}
    } 
  else if (strcasecmp (name, "matchbox/background") == 0
	   && flag_rows)
    {
      if (setting && setting->type == XSETTINGS_TYPE_STRING)
	{
	  enum image_type type = SCALED;
	  gchar *path = setting->data.v_string;
	  if (!strncmp (path, "img-tiled:", 10))
	    {
	      path += 10;
	      type = TILED;
	    }
	  else if (!strncmp (path, "img-stretched:", 14))
	    {
	      path += 14;
	      type = SCALED;
	    }
	  else if (!strncmp (path, "img-centered:", 13))
	    {
	      path += 13;
	      type = CENTERED;
	    }	  
	  row_view_set_background (path);
	}
      else if (setting == NULL)
	{
	  row_view_set_background (NULL);
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
watch_func (Window window,
	    Bool   is_start,
	    long   mask,
	    void  *cb_data)
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
gpe_appmgr_start_xsettings (void)
{
  Display *dpy = GDK_DISPLAY ();

  client = xsettings_client_new (dpy, DefaultScreen (dpy), notify_func, watch_func, NULL);
  if (client == NULL)
    {
      fprintf (stderr, "Cannot create XSettings client\n");
      return FALSE;
    }

  return TRUE;
}
