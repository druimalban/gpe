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

/* Management of some dialog boxes */

#include "olirc.h"
#include "windows.h"
#include "servers.h"
#include "network.h"
#include "dialogs.h"
#include "misc.h"
#include "channels.h"
#include "prefs.h"
#include "servers_dialogs.h"

#include <gdk/gdkkeysyms.h>
#include <string.h>

enum { A_CANCEL, A_OK, A_MOVE_TO_GW, A_CREATE_NEW_GW, A_DISCONNECT };

/* ----- Some useful functions -------------------------------------------------------- */

gint dialog_simple_key_press_event(GtkWidget *w, GdkEventKey *v, gpointer data)
{
	g_return_val_if_fail(data, FALSE);

	if (v->keyval == GDK_Escape)
	{
		gtk_widget_destroy((GtkWidget *) data);
		return TRUE;
	}

	return FALSE;
}

gint dialog_key_press_event(GtkWidget *w, GdkEventKey *v, gpointer data)
{
	struct dbox *dbox = (struct dbox *) data;

	g_return_val_if_fail(dbox, FALSE);

	switch (v->keyval)
	{
		case GDK_Escape:
			gtk_widget_destroy(dbox->window);
			return TRUE;
		case GDK_Tab:
		case GDK_Up:
		case GDK_Down:
			if (dbox->stop_focus)
			{
				gtk_signal_emit_stop_by_name((GtkObject *) w, "key_press_event");
				return TRUE;
			}
			break;
		case GDK_Return:
		case GDK_KP_Enter:
			if (dbox->return_func)
			{
				dbox->return_func(data);
				return TRUE;
			}
	}

	return FALSE;
}

GtkWidget *Pack_Button(GtkDialog *dialog, gchar *button_text, void function(GtkWidget *, gint), gint data)
{
	/* Add a labeled button in a pack box */

	GtkWidget *button;

	button = gtk_button_new_with_label(button_text);
	GTK_WIDGET_UNSET_FLAGS(button, GTK_CAN_FOCUS);
	gtk_signal_connect(GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC(function), (gpointer) data);
	gtk_box_pack_start(GTK_BOX(dialog->action_area), button, TRUE, TRUE, 10);
	gtk_widget_show(button);

	return button;
}

void Add_Dialog_Table_Button(GtkWidget *table, gchar *button_text, void function(GtkWidget *, gint), gint data, gint x, gint y)
{
	/* Add a labeled button in a table */

	GtkWidget *button;

	button = gtk_button_new_with_label(button_text);
	GTK_WIDGET_UNSET_FLAGS(button, GTK_CAN_FOCUS);
	gtk_signal_connect(GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC(function), (gpointer) data);
	gtk_table_attach(GTK_TABLE(table), button, x, x+1, y, y+1, GTK_FILL | GTK_EXPAND, 0, 10, 0);
	gtk_widget_show(button);
}

GtkWidget *Add_Dialog_Table_Entry(GtkWidget *table, gchar *label_text, gint x, gint y)
{
	/* Add a label and a text entry in two cells of a table */

	GtkWidget *label;
	GtkWidget *entry;

	label = gtk_label_new(label_text);
	gtk_table_attach(GTK_TABLE(table), label, x, x+1, y, y+1, 0, 0, 10, 0);
	gtk_widget_show(label);
	entry = gtk_entry_new();
	gtk_widget_show(entry);
	gtk_table_attach_defaults(GTK_TABLE(table), entry, x+1, x+2, y, y+1);

	return entry;	/* Return the entry widget */
}

struct msg_box_struct
{
	struct dbox dbox;
};

void Message_Box(gchar *title, gchar *text)
{
	/* Display a dialog box with an (error) message */

	struct msg_box_struct *mbs;

	GtkWidget *b = gtk_button_new_with_label(" Ok ");
	GtkWidget *label, *vbox, *ftmp, *btmp;
	gchar *tmp = (gchar *) g_malloc(strlen(text) + 2);
	gchar *seek;

	mbs = (struct msg_box_struct *) g_malloc0(sizeof(struct msg_box_struct));

	mbs->dbox.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_position((GtkWindow *) mbs->dbox.window, GTK_WIN_POS_MOUSE);
	gtk_window_set_policy((GtkWindow *) mbs->dbox.window, FALSE, FALSE, TRUE);

	strcpy(tmp, text);
	seek = strtok(tmp,"\n");

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add((GtkContainer *) mbs->dbox.window, vbox);

	ftmp = gtk_frame_new(NULL);
	gtk_container_border_width((GtkContainer *) ftmp, 2);
	gtk_box_pack_start((GtkBox *) vbox, ftmp, TRUE, TRUE, 0);

	btmp = gtk_vbox_new(TRUE, 0);
	gtk_container_add((GtkContainer *) ftmp, btmp);
	gtk_container_border_width((GtkContainer *) btmp, 16);

	do
	{
		label = gtk_label_new(seek);
		gtk_box_pack_start((GtkBox *) btmp, label, TRUE, TRUE, 2);
	} while ((seek = strtok(NULL,"\n")));

	btmp = gtk_hbox_new(TRUE, 0);
	gtk_container_border_width((GtkContainer *) btmp, 6);
	gtk_box_pack_start((GtkBox *) vbox, btmp, FALSE, FALSE, 0);

	gtk_box_pack_start((GtkBox *) btmp, b, TRUE, TRUE, 40);
	GTK_WIDGET_UNSET_FLAGS(b, GTK_CAN_FOCUS);
	GTK_WIDGET_SET_FLAGS(b, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(b);

	gtk_window_set_title((GtkWindow *) mbs->dbox.window, title);
	gtk_signal_connect_object((GtkObject *) b, "clicked", (GtkSignalFunc) gtk_widget_destroy, (GtkObject *) mbs->dbox.window);
	gtk_signal_connect((GtkObject *) mbs->dbox.window, "key_press_event", (GtkSignalFunc) dialog_key_press_event, (gpointer) mbs);

	gtk_widget_show_all(mbs->dbox.window);
	gtk_grab_add(mbs->dbox.window);
}

/* ----- Socks preferences dialog box ------------------------------------------------- */

GtkWidget *Socks_Box = NULL;
GtkWidget *Socks_Host, *Socks_Port, *Socks_User;
GtkWidget *Socks_OnOff;

void Dialog_Socks_delete_event(GtkWidget *widget, GdkEvent *event, gpointer dummy)
{
	gtk_widget_destroy(Socks_Box);
	Socks_Box = NULL;
}

void Dialog_Socks_Action(GtkWidget *w, gint action)
{
	if (action==A_OK)
	{
		g_free(GPrefs.Socks_Host);
		g_free(GPrefs.Socks_User);

		GPrefs.Socks_Host = g_strdup(gtk_entry_get_text(GTK_ENTRY(Socks_Host)));
		GPrefs.Socks_Port = atoi(gtk_entry_get_text(GTK_ENTRY(Socks_Port)));
		GPrefs.Socks_User = g_strdup(gtk_entry_get_text(GTK_ENTRY(Socks_User)));

		if (GTK_TOGGLE_BUTTON(Socks_OnOff)->active) GPrefs.Flags |= PREF_USE_SOCKS;
		else GPrefs.Flags &= ~(PREF_USE_SOCKS);
	}

	gtk_widget_destroy(Socks_Box);
	Socks_Box = NULL;
}

void Dialog_Socks()
{
	GtkWidget *vbox, *table, *wtmp;
	gchar ptmp[32];

	if (Socks_Box)
	{
		gdk_window_raise(Socks_Box->window); 
		return;
	}

	Socks_Box = gtk_dialog_new();

	gtk_window_set_title(GTK_WINDOW(Socks_Box), "Socks preferences");

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(Socks_Box)->vbox), vbox, TRUE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 15);

	table = gtk_table_new(3, 6, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 10);

	wtmp = gtk_label_new("If you are behind a SOCKS firewall, Ol-irc can\nconnect through it (using the SOCKS V4 protocol for now).");
	gtk_table_attach(GTK_TABLE(table), wtmp, 0, 3, 0, 1, 0, 0, 0, 0);

	Socks_OnOff = gtk_check_button_new_with_label(" Use Socks Server for connections");
	gtk_table_attach(GTK_TABLE(table), Socks_OnOff, 0, 3, 2, 3, 0, 0, 0, 8);
	GTK_WIDGET_UNSET_FLAGS(Socks_OnOff, GTK_CAN_FOCUS);
	if (GPrefs.Flags & PREF_USE_SOCKS) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(Socks_OnOff), TRUE);

	Socks_Host = Add_Dialog_Table_Entry(table, "Socks server", 0, 3);
	gtk_entry_set_text(GTK_ENTRY(Socks_Host), GPrefs.Socks_Host);
	Socks_Port = Add_Dialog_Table_Entry(table, "Socks port", 0, 4);
	sprintf(ptmp, "%d", GPrefs.Socks_Port);
	gtk_entry_set_text(GTK_ENTRY(Socks_Port), ptmp);
	Socks_User = Add_Dialog_Table_Entry(table, "Socks user", 0, 5);
	gtk_entry_set_text(GTK_ENTRY(Socks_User), GPrefs.Socks_User);

	Pack_Button(GTK_DIALOG(Socks_Box), "Ok", Dialog_Socks_Action, A_OK);
	Pack_Button(GTK_DIALOG(Socks_Box), "Cancel", Dialog_Socks_Action, A_CANCEL);

	gtk_signal_connect(GTK_OBJECT(Socks_Box), "delete_event", GTK_SIGNAL_FUNC(Dialog_Socks_delete_event), NULL);

	gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);

	gtk_widget_show_all(Socks_Box);
	gtk_widget_grab_focus(Socks_Host);
	gtk_grab_add(Socks_Box);
}

/* ------------------------------------------------------------------------------------- */

void Filesel_Home_clicked(GtkWidget *w, gpointer data)
{
	GtkFileSelection *fs = (GtkFileSelection *) data;
	gchar *f = gtk_file_selection_get_filename(fs);
	gchar *l = strrchr(f, '/');
	gchar d_tmp[1024];
	if (l) l++; else l = f;
	if (*l) sprintf(d_tmp, "%s/%s", Olirc->home_path, l);
	else sprintf(d_tmp, "%s/", Olirc->home_path);
	gtk_file_selection_set_filename(fs, d_tmp);
}

void Add_Home_Button(GtkFileSelection *fs)
{
	GtkWidget *button;
	GtkWidget *btmp;

	g_return_if_fail(fs);

	btmp = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(btmp), GTK_BUTTONBOX_END);
	gtk_box_pack_end((GtkBox *) fs->button_area, btmp, TRUE, TRUE, 0);

	button = gtk_button_new_with_label("Home");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(Filesel_Home_clicked), (gpointer) fs);
	gtk_box_pack_end((GtkBox *) btmp, button, TRUE, TRUE, 0);

	gtk_widget_show(btmp);
	gtk_widget_show(button);
}

/* ------------------------------------------------------------------------------------- */

void olirc_real_quit()
{




	gtk_main_quit();
}

void olirc_quit(gchar *reason, gboolean ask)
{
	/* Quit Ol-Irc */

	GList *g = NULL;

	if (Servers_List) /* Find all connected servers */
	{
		GList *l;
		Server *s;

		l = Servers_List;
		while (l)
		{
			s = (Server *) l->data; l = l->next;
			if (s->State != SERVER_IDLE && s->State != SERVER_DISCONNECTING) g = g_list_append(g, (gpointer) s);
		}
	}

	if (g && ask)
	{
		dialog_quit_servers(g, reason, NULL, NULL, TRUE);
		return;
	}

	Olirc->Flags |= FLAG_QUITING;

	if (!reason) reason = GPrefs.Quit_Reason;
	else g_free_and_set(GPrefs.Quit_Reason, g_strdup(reason));

	Prefs_Update();

	while (g) { Server_Quit((Server *) g->data, reason); g = g_list_remove(g, g ->data); }

 	olirc_real_quit();
}

/* vi: set ts=3: */

