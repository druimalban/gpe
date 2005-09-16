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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "package.h"
#include "popupmenu.h"
#include "properties.h"
#include "main.h"

static int menu_timeout_id=0;

static int on_popdown (GtkWidget *menuitem, gpointer data)
{
	gpe_iconlist_popup_removed (data);
	return 0;
}

static int on_menuitem_properties (GtkWidget *menuitem, gpointer data)
{
	struct package *p;

	p = (struct package *) data;

	show_properties (p);

	return FALSE;
}

void trim_text (GdkFont *font, char *text, int px)
{
	int i;

	for (i=0;i<strlen(text);i++)
	{
		if (gdk_text_width (font, text, i) > px)
		{
			/* Cut the title short & make it end in "..." */
			int j;
			if (i > 0)
				*(text + --i) = 0;
			for (j=0;j<3;j++)
				*(text + --i) = '.';
			return;
		}
	}
}

/* Long-hold popup menu */
int popup_menu_activate (gpointer data, GtkWidget *il)
{
	struct package *p;
	GtkWidget *menu, *mi;

	p = (struct package *)data;

	menu = gtk_menu_new ();

	mi = gtk_menu_item_new_with_label ("Properties");
	gtk_signal_connect( GTK_OBJECT(mi), "activate",
			    GTK_SIGNAL_FUNC(on_menuitem_properties), p);
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), mi);

	gtk_widget_show_all (menu);

	gtk_signal_connect( GTK_OBJECT(menu), "deactivate",
			    GTK_SIGNAL_FUNC(on_popdown), il);

	gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, 0);

	return FALSE;
}

void popup_menu_cancel (void)
{
	if (menu_timeout_id != 0)
	{
		gtk_timeout_remove (menu_timeout_id);
		menu_timeout_id = 0;
	}
}
