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

#ifndef CFG_H
#define CFG_H

#include "main.h"

/* View types for the tabs */
typedef enum
{
        TAB_VIEW_ICONS,
        TAB_VIEW_LIST,
} tab_view_style;

struct cfg_options_s
{
	int show_all_group;
	int auto_hide_group_labels;
	int show_recent_apps;
	int recent_apps_number;
	int list_icon_size;
	tab_view_style tab_view;
} cfg_options;

void cfg_load ();
void cfg_load_if_newer (time_t when);

#endif
