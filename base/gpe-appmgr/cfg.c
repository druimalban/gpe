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

#include "cfg.h"
#include "package.h"

void cfg_load ()
{
	struct package *p=NULL;
	char *home_dir;
	char config_path[]=".gpe/gpe-appmgr";
	char *s;

	/* Default options */
	cfg_options.show_all_group = 0;
	cfg_options.auto_hide_group_labels = 1;
	cfg_options.tab_view = TAB_VIEW_ICONS;
	cfg_options.recent_apps = -1;

	home_dir = (char*) getenv("HOME");
	if (home_dir)
	{
		s = (char*) malloc (strlen (home_dir) + strlen (config_path) + 1);
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

	/* How many 'recent' apps to have */
	if ((s = package_get_data (p, "recent_apps")))
	{
		sscanf(s, "%d", cfg_options.recent_apps);
	}

	if ((s = package_get_data (p, "tab_view")))
	{
		switch (tolower(*s))
		{
		case 'l':
			cfg_options.tab_view = TAB_VIEW_LIST;
		}
	}
}
