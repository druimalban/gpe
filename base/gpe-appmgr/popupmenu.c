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
#include "xsi.h"
#include "main.h"

static int menu_timeout_id=0;

static int on_menuitem_force_run (GtkWidget *menuitem, gpointer data)
{
	struct package *p;

	p = (struct package *) data;
	run_program (package_get_data (p, "command"), NULL);

	return FALSE;
}

static int on_menuitem_raise (GtkWidget *menuitem, gpointer data)
{
	raise_window_default ((Window) data);

	return FALSE;
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
int popup_menu_activate (gpointer data)
{
	struct package *p;
	GtkWidget *menu, *mi;
	GList *l=NULL, *r=NULL;
	char *window_title;
#ifndef GTK2
	GdkFont *font; /* Font in the menu label */
#endif
	p = (struct package *)data;

	menu = gtk_menu_new ();
#ifndef GTK2
	if (gtk_rc_get_style (menu))
		font = gtk_rc_get_style (menu)->font;
	else
		font = gtk_widget_get_style (menu)->font;
#endif
	window_title = package_get_data (p, "windowtitle");
	if (window_title)
		r = l = get_windows_with_name (window_title);
	while (l)
	{
		struct window_info *w;

		w = (struct window_info *) l->data;
#ifndef GTK2
		trim_text (font, w->name, 100);
#endif
		mi = gtk_menu_item_new_with_label (w->name);
		gtk_signal_connect( GTK_OBJECT(mi), "activate",
				    GTK_SIGNAL_FUNC(on_menuitem_raise), (gpointer)w->xid);
		gtk_menu_append (GTK_MENU(menu), mi);

		window_info_free (w);

		l=l->next;
	}

	if (r)
	{
		g_list_free (r);
		gtk_menu_append (GTK_MENU(menu), gtk_menu_item_new());
	}

	mi = gtk_menu_item_new_with_label ("Force run");
	gtk_signal_connect( GTK_OBJECT(mi), "activate",
			    GTK_SIGNAL_FUNC(on_menuitem_force_run), p);
	gtk_menu_append (GTK_MENU(menu), mi);

	mi = gtk_menu_item_new_with_label ("Properties");
	gtk_signal_connect( GTK_OBJECT(mi), "activate",
			    GTK_SIGNAL_FUNC(on_menuitem_properties), p);
	gtk_menu_append (GTK_MENU(menu), mi);

	gtk_widget_show_all (menu);

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

void popup_menu_later (int delay, gpointer data)
{
	menu_timeout_id = gtk_timeout_add (delay, popup_menu_activate, data);
}
