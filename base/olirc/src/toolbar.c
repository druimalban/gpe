/*
 * ol-irc - A small irc client using GTK+
 *
 * Copyright (C) 1998, 1999 Yann Grossel [Olrick]
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

/* Creation, setup and management of the toolbars */

#include "olirc.h"
#include "menus.h"
#include "windows.h"
#include "servers.h"
#include "icons.h"

#include <gtk/gtk.h>
#include <gtk/gtkstyle.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

void Toolbar_click(GtkWidget *widget, gpointer data)
{
	g_print("[NOT IMPLEMENTED]\n");
}

void Toolbar_Switch(GUI_Window *rw)
{
	/* Hide the toolbar if it is visible, else show it */

	if (rw->Toolbar_visible)
	{
		if (!rw->Menus_visible) gtk_widget_hide(rw->sep1);
		gtk_widget_hide(rw->Toolbar_Box);
		gtk_widget_hide(rw->sep2);
	}
	else
	{
		if (!rw->Menus_visible) gtk_widget_show(rw->sep1);
		gtk_widget_show(rw->Toolbar_Box);
		gtk_widget_show(rw->sep2);
	}

	rw->Toolbar_visible = !rw->Toolbar_visible;
}
     
void Toolbar_Init(GUI_Window *rw)
{
	/* Initialize the Toolbar of a window */

	GtkWidget *button, *separator;
	GtkTooltips *tt;
	int i;

	tt = gtk_tooltips_new();

	/* We first put a blank label widget in the toolbar */

	button = gtk_label_new(" ");
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(rw->Toolbar_Box), button, FALSE, FALSE, 0);

	{
		/* --- XXX THIS MUST BE ENTIRELY REWRITTEN XXX --- */

		GdkPixmap *pixmap=NULL;
		GdkBitmap *mask=NULL;
		GtkStyle *style=NULL;
		GtkWidget *pixmapwid;

		/* The "New Server" button */

		button = gtk_button_new();
		GTK_WIDGET_UNSET_FLAGS(button, GTK_CAN_FOCUS);
		gtk_signal_connect(GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (Toolbar_click), NULL);
		gtk_tooltips_set_tip(tt, button, "Connect to a server", NULL);
	   gtk_widget_show(button);
		style = gtk_widget_get_style(button);
		pixmap = gdk_pixmap_create_from_xpm_d(rw->Gtk_Window->window,&mask,&style->fg[GTK_STATE_NORMAL],Server_xpm);
		pixmapwid = gtk_pixmap_new(pixmap, mask);
		gtk_widget_show(pixmapwid);
		gtk_container_add(GTK_CONTAINER(button), pixmapwid);
		gtk_box_pack_start(GTK_BOX (rw->Toolbar_Box), button, FALSE, FALSE, 0);

		/* The "Prefs" button */

		button = gtk_button_new();
		GTK_WIDGET_UNSET_FLAGS(button, GTK_CAN_FOCUS);
		gtk_signal_connect(GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (Toolbar_click), NULL);
		gtk_tooltips_set_tip(tt, button, "Change the preferences", NULL);
	   gtk_widget_show(button);
		style = gtk_widget_get_style(button);
		pixmap = gdk_pixmap_create_from_xpm_d(rw->Gtk_Window->window,&mask,&style->fg[GTK_STATE_NORMAL],prefs_xpm);
		pixmapwid = gtk_pixmap_new(pixmap, mask);
		gtk_widget_show(pixmapwid);
		gtk_container_add(GTK_CONTAINER(button), pixmapwid);
		gtk_box_pack_start(GTK_BOX (rw->Toolbar_Box), button, FALSE, FALSE, 0);
	}

	for (i=0; i<16; i++)
	{
		button = gtk_button_new_with_label("     ");
		GTK_WIDGET_UNSET_FLAGS(button, GTK_CAN_FOCUS);
		gtk_signal_connect(GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (Toolbar_click), NULL);
	   gtk_widget_show(button);
		gtk_box_pack_start(GTK_BOX (rw->Toolbar_Box), button, FALSE, FALSE, 0);
		if (i%4==3 && i<15)
		{
			separator = gtk_vseparator_new();
			gtk_widget_show(separator);
			gtk_box_pack_start(GTK_BOX (rw->Toolbar_Box), separator, FALSE, FALSE, 5);
		}
	}

}

/* vi: set ts=3: */

