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

#include <gdk/gdkx.h>

#include "xsettings-client.h"
#include "main.h"
#include "cfg.h"

static XSettingsClient *client;

#define KEY_BASE "GPE/APPMGR/"

static void
notify_func (const char       *name,
	     XSettingsAction   action,
	     XSettingsSetting *setting,
	     void             *cb_data)
{
//	printf ("notify!\n");
//	printf ("SET %s\n", name);

#if 0
  if (strncmp (name, KEY_BASE, strlen (KEY_BASE)) == 0)
    {
      char *p = (char*)name + strlen (KEY_BASE);

      if (!strcmp (p, "SHOW-ALL-GROUP"))
	{
	  if (setting->type == XSETTINGS_TYPE_INT)
	    {

		    if (cfg_options.show_all_group)
			    gtk_notebook_remove_page (GTK_NOTEBOOK(notebook), 0);

		    cfg_options.show_all_group = setting->data.v_int;

		    create_all_tab();

		    gtk_notebook_set_page (GTK_NOTEBOOK(notebook), 0);
	    }
	}

      if (!strcmp (p, "AUTOHIDE-GROUP-LABELS"))
      {
	      if (setting->type == XSETTINGS_TYPE_INT)
	      {
		      cfg_options.auto_hide_group_labels = setting->data.v_int;
		      
		      /* Refresh view... */
		      autohide_labels (-1);
	      }
      }
      
      if (!strcmp (p, "SHOW-RECENT-APPS")) {
	      if (setting->type == XSETTINGS_TYPE_INT) {
		      cfg_options.show_recent_apps = setting->data.v_int;
		      create_recent_box();
	      }
      }

      if (!strcmp (p, "ON-WINDOW-CLOSE")) {
	      if (setting->type == XSETTINGS_TYPE_STRING) {
		      if (!strcasecmp(setting->data.v_string, "exit"))
			      cfg_options.on_window_close = WINDOW_CLOSE_EXIT;
		      else
			      cfg_options.on_window_close = WINDOW_CLOSE_IGNORE;
	      }
      }

    }
#endif
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

	/* Default options */
	cfg_options.show_all_group = 0;
	cfg_options.auto_hide_group_labels = 1;
	cfg_options.tab_view = TAB_VIEW_ICONS;
	cfg_options.list_icon_size = 48;
	cfg_options.show_recent_apps = 0;
	cfg_options.recent_apps_number = 4;
	cfg_options.on_window_close = WINDOW_CLOSE_IGNORE;
	cfg_options.use_windowtitle = 1;

  client = xsettings_client_new (dpy, DefaultScreen (dpy), notify_func, watch_func, NULL);
  if (client == NULL)
    {
      fprintf (stderr, "Cannot create XSettings client\n");
      return FALSE;
    }

//  push_new_changes = TRUE;

	return TRUE;
}
