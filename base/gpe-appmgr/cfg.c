/* gpe-appmgr - a program launcher

   Copyright 2002 Robert Mibus;

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* For stat() */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cfg.h"
#include "package.h"
//#include "xsettings.c"

void cfg_load ()
{
#if 0
	struct package *p=NULL;
	char *home_dir;
	char config_path[]=".gpe/gpe-appmgr";
	char *s;
#endif

	/* Default options */
	cfg_options.show_all_group = 0;
	cfg_options.auto_hide_group_labels = 1;
	cfg_options.tab_view = TAB_VIEW_ICONS;
	cfg_options.list_icon_size = 48;
	cfg_options.show_recent_apps = 0;
	cfg_options.recent_apps_number = 4;
	cfg_options.on_window_close = WINDOW_CLOSE_IGNORE;
	cfg_options.use_windowtitle = 1;

#if 0
	home_dir = (char*) getenv("HOME");
	if (home_dir)
	{
		s = (char*) malloc (strlen (home_dir) + strlen (config_path) + 2);
		sprintf (s, "%s/%s", home_dir, config_path);
		p = package_read (s);
		free (s);
	}

	if (!p)
		p = package_read ("/usr/share/gpe/config/gpe-appmgr");

	/* Whether to show the "All" pseudo-group */
	if ((s = package_get_data (p, "show_all_group")))
	{
		switch (tolower(*s))
		{
		case '1':
		case 'y':
		case 't':
			cfg_options.show_all_group = 1;
		}
	}

	/* Whether to hide group labels when not active */
	if ((s = package_get_data (p, "auto_hide_group_labels")))
	{
		switch (tolower(*s))
		{
		case '1':
		case 'y':
		case 't':
			break;
		default:
			cfg_options.auto_hide_group_labels = 0;
		}
	}

	/* Whether to have a 'Recent' tab */
	if ((s = package_get_data (p, "show_recent_apps")))
	{
		switch (tolower(*s))
		{
		case '1':
		case 'y':
		case 't':
			cfg_options.show_recent_apps = 1;
			break;
		}
	}

	/* What to do when the user tries to close us */
	if ((s = package_get_data (p, "on_window_close")))
	{
		switch (tolower(*s))
		{
		case 'H': case 'h': /* doesn't work properly! */
			cfg_options.on_window_close = WINDOW_CLOSE_HIDE;
			break;
		case 'E': case 'e':
			cfg_options.on_window_close = WINDOW_CLOSE_EXIT;
			break;
		}
	}

	/* Use windowtitle=? */
	if ((s = package_get_data (p, "use_windowtitle")))
	{
		switch (tolower(*s))
		{
		case '0':
		case 'N': case 'n':
		case 'F': case 'f':
			cfg_options.use_windowtitle = 0;
			break;
		}
	}

	if ((s = package_get_data (p, "tab_view")))
	{
		switch (tolower(*s))
		{
		case 'l':
			cfg_options.tab_view = TAB_VIEW_LIST;
		}
	}

	package_free (p);
#endif
}

void cfg_load_if_newer (time_t when)
{
#if 0
	struct stat buf;
	const char *home;

	if ((home = g_get_home_dir ())) {
		char *s;

		/* Check how new the config file is */
		s = g_strdup_printf ("%s/.gpe/gpe-appmgr", home);
		stat (s, &buf);
		g_free (s);

		/* Run away if it's older */
		if (buf.st_mtime <= when)
			return;
	}

	cfg_load();
#endif
}
