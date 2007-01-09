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

#include "olirc.h"
#include "dialogs.h"

#include <gdk/gdkkeysyms.h>

enum { A_CANCEL, A_OK };

#define Number_of_Events 7

GtkWidget *Channel_Events_Box = NULL;
GtkWidget *tb[Number_of_Events];
Channel *Channel_Events_Channel;

static struct { gchar *name; gint pref; } events[] =
{
	{ "JOIN", PREF_CHAN_VIEW_JOIN },
	{ "PART", PREF_CHAN_VIEW_PART },
	{ "QUIT", PREF_CHAN_VIEW_QUIT },
	{ "NICK", PREF_CHAN_VIEW_NICK },
	{ "KICK", PREF_CHAN_VIEW_KICK },
	{ "MODE", PREF_CHAN_VIEW_MODE },
	{ "TOPIC", PREF_CHAN_VIEW_TOPIC }
};

void Dialog_Channel_Events_delete_event(GtkWidget *widget, GdkEvent *event, gpointer dummy)
{
	gtk_widget_destroy(Channel_Events_Box);
	Channel_Events_Box = NULL;
}

void Dialog_Channel_Events_Action(GtkWidget *w, gint action)
{
	if (action==A_OK)
	{
		gint i;

		for (i=0; i<Number_of_Events; i++)
		{
			if (GTK_TOGGLE_BUTTON(tb[i])->active) Channel_Events_Channel->Prefs |= events[i].pref;
			else Channel_Events_Channel->Prefs &= ~events[i].pref;
		}
	}

	gtk_widget_destroy(Channel_Events_Box);
	Channel_Events_Box = NULL;
}

gint Dialog_Channel_Events_key_press_event(GtkObject *w, GdkEventKey *v, gpointer d)
{
	switch (v->keyval)
	{
		case GDK_Escape: Dialog_Channel_Events_Action(NULL, A_CANCEL); break;
		case GDK_Return:
		case GDK_KP_Enter: Dialog_Channel_Events_Action(NULL, A_OK); break;
	}
	return FALSE;
}

void Dialog_Channel_Events(Channel *c)
{
	GtkWidget *widget;
	GtkWidget *vbox;
	gint i;
	gchar tmp[128];
	
	if (Channel_Events_Box)
	{
		gdk_window_raise(Channel_Events_Box->window); 
		return;
	}

	g_return_if_fail(c);

	Channel_Events_Channel = c;

	Channel_Events_Box = gtk_dialog_new();

	gtk_window_set_title(GTK_WINDOW(Channel_Events_Box), "Displaying of messages");

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(Channel_Events_Box)->vbox), vbox, TRUE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 15);

	if (c->server->fs->Network) sprintf(tmp,"Channel %s on server %s (network %s)", c->Name, c->server->fs->Name, c->server->fs->Network);
	else sprintf(tmp,"Channel %s on server %s", c->Name, c->server->fs->Name);
	widget = gtk_label_new(tmp);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);

	widget = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);

	widget = gtk_label_new("Please choose the messages that you want to be displayed:");
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);

	for (i=0; i<Number_of_Events; i++)
	{
		sprintf(tmp,"Display %s messages", events[i].name);
		tb[i] = gtk_check_button_new_with_label(tmp);
		GTK_WIDGET_UNSET_FLAGS(tb[i], GTK_CAN_FOCUS);
		if (c->Prefs&events[i].pref) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(tb[i]), TRUE);
		gtk_box_pack_start(GTK_BOX(vbox), tb[i], TRUE, TRUE, 0);
	}

	Pack_Button(GTK_DIALOG(Channel_Events_Box), " Ok ", Dialog_Channel_Events_Action, A_OK);
	Pack_Button(GTK_DIALOG(Channel_Events_Box), " Cancel ", Dialog_Channel_Events_Action, A_CANCEL);

	gtk_signal_connect(GTK_OBJECT(Channel_Events_Box), "delete_event", GTK_SIGNAL_FUNC(Dialog_Channel_Events_delete_event), NULL);
	gtk_signal_connect(GTK_OBJECT(Channel_Events_Box), "key_press_event", GTK_SIGNAL_FUNC(Dialog_Channel_Events_key_press_event), NULL);

	gtk_widget_show_all(Channel_Events_Box);
	gtk_grab_add(Channel_Events_Box);
}

/* vi: set ts=3: */

