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
#include "ignores.h"
#include "misc.h"
#include "dialogs.h"
#include "windows.h"
#include "network.h"
#include "servers.h"
#include "numerics.h"

#include <string.h>

gchar itmp[1024];

/* ------------------------------------------------------------------------------------- */

gboolean Ignore_Check(Server *s, gchar *user, guint msg_type)
{
	register GList *l;
	struct Ignore *ign;
	gboolean res = FALSE;

	g_return_val_if_fail(s, FALSE);
	g_return_val_if_fail(user, FALSE);

	l = s->Ignores;

	while (l)
	{
		ign = (struct Ignore *) l->data;

		if (mask_match(ign->Mask, user))
		{
			if (ign->Flags & IGNORE_EXCLUDE) return FALSE;
			else if (ign->Flags & msg_type) res = TRUE;
		}

		l = l->next;
	}

	return res;
}

/* ----- Structures used by the ignores dialog boxes ---------------------------------- */

static gchar *ilbox_clist_titles[] =
{
	"Ignore mask",
	"Expiry",
	"Private",
	"Channel",
	"Invite",
	"Notice",
	"CTCP",
	"Exclude"
};

struct Ignore_List_Box
{
	struct dbox dbox;

	Server *server;
	GtkWidget *add_button;
	GtkWidget *remove_button;
	GtkWidget *edit_button;
	GtkWidget *clist;
	gint row_selected;
};

struct Ignore_Box
{
	struct dbox dbox;
	struct Ignore *ignore;
	GtkWidget *combo_mask;
	GtkWidget *check_button[5];
	GtkWidget *all_button;
	GtkWidget *Label_1, *Label_2;
	GtkWidget *entry_lifetime;
	GtkWidget *expire_on_quit_button;
	GtkWidget *exclude_button;
	GList *strings;
};

gchar *ignore_add_check_buttons_names[] =
{
	"Ignore private messages",
	"Ignore channels messages",
	"Ignore invites",
	"Ignore notices",
	"Ignore CTCP"
};

GList *Ignores_Not_Linked = NULL;

/* ----- Functions related to servers and ignores -------------------------------------- */

void Server_Ignores_Destroy(Server *s)
{
	struct Ignore *ign;
	GList *l;

	g_return_if_fail(s);

	if (s->ignore_list_box)
		gtk_widget_destroy(((struct Ignore_List_Box *) s->ignore_list_box)->dbox.window);

	while (s->Ignores)
	{
		ign = (struct Ignore *) s->Ignores->data;

		if (ign->ignore_box)
			gtk_widget_destroy(((struct Ignore_Box *) ign->ignore_box)->dbox.window);
		
		s->Ignores = g_list_remove(s->Ignores, s->Ignores->data);
		g_free(ign->Mask); g_free(ign);
	}

	l = Ignores_Not_Linked;

	while (l)
	{
		ign = (struct Ignore *) l->data;
		l = l->next;

		if (ign->server == s)
		{
			if (!(ign->ignore_box))
			{
				g_warning("No Ignore Box for not linked ignore %p !?!?\n", ign);
				Ignores_Not_Linked = g_list_remove(Ignores_Not_Linked, (gpointer) ign);
				g_free(ign->Mask); g_free(ign);
			}
			else gtk_widget_destroy(((struct Ignore_Box *) ign->ignore_box)->dbox.window);
		}
	}
}

void Server_Ignore_Remove(Server *s, struct Ignore *ign, gboolean expired)
{
	struct Ignore_List_Box *ilbox;
	struct Ignore_Box *ibox;

	g_return_if_fail(s);
	g_return_if_fail(ign);

	ilbox = (struct Ignore_List_Box *) s->ignore_list_box;
	ibox = (struct Ignore_Box *) ign->ignore_box;

	if (ibox)
	{
		gtk_widget_destroy(ibox->dbox.window);
		if (expired) sprintf(itmp,"Ignore %s has expired.", ign->Mask);
		else sprintf(itmp,"Ignore %s has been removed.", ign->Mask);
		Message_Box(ign->server->fs->Name, itmp);
	}

	if (ilbox)
	{
		gint row = gtk_clist_find_row_from_data((GtkCList *) ilbox->clist, (gpointer) ign);
		if (row != -1) gtk_clist_remove((GtkCList *) ilbox->clist, row);
	}

	ign->server->Ignores = g_list_remove(ign->server->Ignores, (gpointer) ign);
	g_free(ign->Mask); g_free(ign);
}

/* ----- Function called if an ignore has expired -------------------------------------- */

void Ignore_Expire(struct Ignore *ign)
{
	g_return_if_fail(ign);
	VW_output(ign->server->vw_active, T_WARNING, "sF-", ign->server, "Ignore mask %s has expired.", ign->Mask);
	Server_Ignore_Remove(ign->server, ign, TRUE);
}

/* ----- Management of Server Ignore List dialog boxes -------------------------------- */

void ilbox_destroy(GtkWidget *w, gpointer data)
{
	struct Ignore_List_Box *ilbox = (struct Ignore_List_Box *) data;
	Server *s;

	g_return_if_fail(ilbox);

	s = ilbox->server;

	g_return_if_fail(s);
	g_return_if_fail(s->ignore_list_box == ilbox);

	g_free_and_NULL(s->ignore_list_box);
}

void ilbox_add_ignore(GtkWidget *widget, gpointer data)
{
	struct Ignore_List_Box *ilbox = (struct Ignore_List_Box *) data;
	g_return_if_fail(ilbox);
	dialog_ignore_properties(ilbox->server, NULL, NULL, "");
}

void ilbox_remove_ignore(GtkWidget *widget, gpointer data)
{
	struct Ignore_List_Box *ilbox = (struct Ignore_List_Box *) data;
	g_return_if_fail(ilbox);
	g_return_if_fail(ilbox->row_selected != -1);
	Server_Ignore_Remove(ilbox->server, (struct Ignore *) gtk_clist_get_row_data((GtkCList *) ilbox->clist, ilbox->row_selected), FALSE);
}

void ilbox_Edit(GtkWidget *widget, gpointer data)
{
	struct Ignore_List_Box *ilbox = (struct Ignore_List_Box *) data;
	struct Ignore *ign;
	g_return_if_fail(ilbox);

	ign = (struct Ignore *) gtk_clist_get_row_data((GtkCList *) ilbox->clist, ilbox->row_selected);
	dialog_ignore_properties(ilbox->server, ign, NULL, NULL);
}

void ilbox_row_selected(GtkWidget *w, gint row, gint column, GdkEventButton *e, gpointer data)
{
	struct Ignore_List_Box *ilbox = (struct Ignore_List_Box *) data;
	g_return_if_fail(ilbox);

	gtk_widget_set_sensitive(ilbox->edit_button, TRUE);
	gtk_widget_set_sensitive(ilbox->remove_button, TRUE);

	ilbox->row_selected = row;
}

void ilbox_row_unselected(GtkWidget *w, gint row, gint column, GdkEventButton *e, gpointer data)
{
	struct Ignore_List_Box *ilbox = (struct Ignore_List_Box *) data;
	g_return_if_fail(ilbox);

	gtk_widget_set_sensitive(ilbox->edit_button, FALSE);
	gtk_widget_set_sensitive(ilbox->remove_button, FALSE);

	ilbox->row_selected = -1;
}

void ilbox_clist_row_set(struct Ignore_List_Box *ilbox, gint row, struct Ignore *ign)
{
	gchar *row_cells[8];
	gint i;

	g_return_if_fail(ilbox);
	g_return_if_fail(ign);

	row_cells[0] = ign->Mask;
	if (ign->expiry_date) row_cells[1] = irc_date(ign->expiry_date);
	else row_cells[1] = "on server closing";
	if (ign->Flags & IGNORE_EXCLUDE)
	{
		for (i=0; i<5; i++) row_cells[i+2] = "";
		row_cells[7] = "X";
	}
	else for (i=0; i<6; i++)
	{
		if (ign->Flags & (1<<i)) row_cells[i+2] = "X";
		else row_cells[i+2] = "";
	}

	if (row == -1)
		row = gtk_clist_append((GtkCList *) ilbox->clist, row_cells);
	else
		for (i=0; i<8; i++)
			gtk_clist_set_text((GtkCList *) ilbox->clist, row, i, row_cells[i]);

	gtk_clist_set_row_data((GtkCList *) ilbox->clist, row, (gpointer) ign);
}

void Ignore_List_New(Server *s)
{
	GtkWidget *wtmp, *vbox, *ftmp, *mvbox, *btmp;
	struct Ignore_List_Box *ilbox = NULL;
	GList *l;
	gint i, wt = 0;

	g_return_if_fail(s);

	if (s->ignore_list_box)
	{
		ilbox = (struct Ignore_List_Box *) s->ignore_list_box;
		g_return_if_fail(ilbox->dbox.window);
		gdk_window_raise(ilbox->dbox.window->window);
		return;
	}

	ilbox = (struct Ignore_List_Box *) g_malloc0(sizeof(struct Ignore_List_Box));
	ilbox->server = s;
	s->ignore_list_box = (gpointer) ilbox;
	ilbox->row_selected = -1;

	ilbox->dbox.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_signal_connect((GtkObject *) ilbox->dbox.window, "destroy", (GtkSignalFunc) ilbox_destroy, (gpointer) ilbox);
	gtk_signal_connect((GtkObject *) ilbox->dbox.window, "key_press_event", (GtkSignalFunc) dialog_key_press_event, (gpointer) ilbox);

	gtk_window_set_position((GtkWindow *) ilbox->dbox.window, GTK_WIN_POS_CENTER);

	sprintf(itmp, "Ignore list of server %s", s->fs->Name);
	gtk_window_set_title((GtkWindow *) ilbox->dbox.window, itmp);

	mvbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add((GtkContainer *) ilbox->dbox.window, mvbox);

	ftmp = gtk_frame_new(NULL);
	gtk_container_border_width((GtkContainer *) ftmp, 2);
	gtk_box_pack_start((GtkBox *) mvbox, ftmp, TRUE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 8);
	gtk_container_add((GtkContainer *) ftmp, vbox);
	gtk_container_border_width((GtkContainer *) vbox, 8);

	ilbox->clist = gtk_clist_new_with_titles(8, ilbox_clist_titles);
	GTK_WIDGET_UNSET_FLAGS(ilbox->clist, GTK_CAN_FOCUS);
	gtk_clist_column_titles_passive((GtkCList *) ilbox->clist);

	{
		gint w;
		GdkFont *f = (ilbox->clist)->style->font;

		for (i=0; i<8; i++)
		{
			w = gdk_text_width(f, ilbox_clist_titles[i], strlen(ilbox_clist_titles[i]));

			if (!i) w += 130;
			else if (i==1) w += 80;
			else gtk_clist_set_column_justification((GtkCList *) ilbox->clist, i, GTK_JUSTIFY_CENTER);

			gtk_clist_set_column_width((GtkCList *) ilbox->clist, i, w);
			wt += w;
		}
	}

	gtk_widget_set_usize(ilbox->clist, wt+62, 200);


	{
		GtkWidget *wtmp = gtk_scrolled_window_new(NULL, NULL);
		gtk_container_add((GtkContainer *) wtmp, ilbox->clist);
		gtk_scrolled_window_set_policy((GtkScrolledWindow *) wtmp, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_box_pack_start((GtkBox *) vbox, wtmp, TRUE, TRUE, 0);
	}

	gtk_signal_connect((GtkObject *) ilbox->clist, "select_row", (GtkSignalFunc) ilbox_row_selected, (gpointer) ilbox);
	gtk_signal_connect((GtkObject *) ilbox->clist, "unselect_row", (GtkSignalFunc) ilbox_row_unselected, (gpointer) ilbox);

	l = s->Ignores;

	while (l)
	{
		ilbox_clist_row_set(ilbox, -1, (struct Ignore *) l->data);
		l = l->next;
	}

	/* The action area */

	btmp = gtk_hbox_new(TRUE, 0);
	gtk_container_border_width((GtkContainer *) btmp, 6);
	gtk_box_pack_start((GtkBox *) mvbox, btmp, FALSE, FALSE, 0);

	ilbox->add_button = gtk_button_new_with_label(" Add an ignore ");
	GTK_WIDGET_UNSET_FLAGS(ilbox->add_button, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, ilbox->add_button, TRUE, TRUE, 16);
	gtk_signal_connect((GtkObject *) ilbox->add_button, "clicked", (GtkSignalFunc) ilbox_add_ignore, (gpointer) ilbox);

	ilbox->remove_button = gtk_button_new_with_label(" Remove this ignore ");
	GTK_WIDGET_UNSET_FLAGS(ilbox->remove_button, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, ilbox->remove_button, TRUE, TRUE, 16);
	gtk_signal_connect((GtkObject *) ilbox->remove_button, "clicked", (GtkSignalFunc) ilbox_remove_ignore, (gpointer) ilbox);
	gtk_widget_set_sensitive(ilbox->remove_button, FALSE);

	ilbox->edit_button = gtk_button_new_with_label(" Edit this ignore ");
	GTK_WIDGET_UNSET_FLAGS(ilbox->edit_button, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, ilbox->edit_button, TRUE, TRUE, 16);
	gtk_signal_connect((GtkObject *) ilbox->edit_button, "clicked", (GtkSignalFunc) ilbox_Edit, (gpointer) ilbox);
	gtk_widget_set_sensitive(ilbox->edit_button, FALSE);

	wtmp = gtk_button_new_with_label(" Close this window ");
	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, wtmp, TRUE, TRUE, 16);
	gtk_signal_connect_object((GtkObject *) wtmp, "clicked", (GtkSignalFunc) gtk_widget_destroy, (gpointer) ilbox->dbox.window);

	gtk_widget_show_all(ilbox->dbox.window);
}

/* ----- Management of Ignore Mask Edit dialog boxes ---------------------------------- */

void ibox_destroy(GtkWidget *w, gpointer data)
{
	struct Ignore_Box *ibox = (struct Ignore_Box *) data;
	struct Ignore *ign;

	g_return_if_fail(ibox);

	ign = ibox->ignore;

	while (ibox->strings)
	{
		g_free(ibox->strings->data);
		ibox->strings = g_list_remove(ibox->strings, ibox->strings->data);
	}

	g_return_if_fail(ign->ignore_box == ibox);

	g_free_and_NULL(ign->ignore_box);

	if (g_list_find(Ignores_Not_Linked, (gpointer) ign))
	{
		Ignores_Not_Linked = g_list_remove(Ignores_Not_Linked, (gpointer) ign);
		g_free(ign->Mask); g_free(ign);
	}
}

void ibox_Ok(GtkWidget *w, gpointer data)
{
	struct Ignore_Box *ibox = (struct Ignore_Box *) data;
	struct Ignore *ign;
	GList *l;
	gint i, f = 0, v = 0;
	gchar *m, *c;
	gboolean one = FALSE, two = FALSE;

	g_return_if_fail(ibox);

	if (((GtkToggleButton *) ibox->exclude_button)->active) f = IGNORE_EXCLUDE;

	for (i=0; i<5; i++)
		if (((GtkToggleButton *) ibox->check_button[i])->active) f |= (1<<i);

	if (!f)
	{
		Message_Box("Error", "You have not chosen any message type.");
		return;
	}

	if (!((GtkToggleButton *) ibox->expire_on_quit_button)->active)
	{
		v = atoi(gtk_entry_get_text((GtkEntry *) ibox->entry_lifetime));

		if (v<=0)
		{
			Message_Box("Error", "The number of minutes is not valid.");
			return;
		}
	}
	else v = 0;

	m = gtk_entry_get_text((GtkEntry *) ((GtkCombo *) ibox->combo_mask)->entry);

	for (c = m; *c; c++)
	{
		if (*c=='!') { if (!one) one = TRUE; else break; }
		if (*c=='@') { if (!two && one) two = TRUE; else break; }
	}

	if (*c || !one || !two)
	{
		Message_Box("Error", "The mask is not valid.");
		return;
	}

	ign = ibox->ignore;
	ign->Flags = f;
	ign->expiry_date = (v)? (time(NULL) + v*60) : 0;
	g_free_and_set(ign->Mask, g_strdup(m));

	l = g_list_find(ign->server->Ignores, (gpointer) ign);

	if (!l)
	{
		ign->server->Ignores = g_list_append(ign->server->Ignores, (gpointer) ign);
		Ignores_Not_Linked = g_list_remove(Ignores_Not_Linked, (gpointer) ign);
	}

	if (ign->server->ignore_list_box)
	{
		gint row;
		struct Ignore_List_Box *ilbox = (struct Ignore_List_Box *) ign->server->ignore_list_box;
		row = gtk_clist_find_row_from_data((GtkCList *) ilbox->clist, (gpointer) ign);
		ilbox_clist_row_set(ilbox, row, ign);
	}

	gtk_widget_destroy(ibox->dbox.window);
}

void ibox_Ignore_All(GtkWidget *w, gpointer data)
{
	struct Ignore_Box *ibox = (struct Ignore_Box *) data;
	gint i;

	g_return_if_fail(ibox);

	for (i=0; i<5; i++)
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(&(GTK_CHECK_BUTTON(ibox->check_button[i]))->toggle_button), TRUE);
}

void ibox_Expire_On_Quit(GtkWidget *w, gpointer data)
{
	struct Ignore_Box *ibox = (struct Ignore_Box *) data;
	gboolean active;

	g_return_if_fail(ibox);

	active = !((GtkToggleButton *) w)->active;

	gtk_widget_set_sensitive(ibox->Label_1, active);
	gtk_widget_set_sensitive(ibox->entry_lifetime, active);
	gtk_widget_set_sensitive(ibox->Label_2, active);
	if (active) gtk_widget_grab_focus(ibox->entry_lifetime);
}

void ibox_Exclude(GtkWidget *w, gpointer data)
{
	struct Ignore_Box *ibox = (struct Ignore_Box *) data;
	gint i;

	g_return_if_fail(ibox);

	for (i=0; i<5; i++)
		gtk_widget_set_sensitive(ibox->check_button[i], !GTK_TOGGLE_BUTTON(w)->active);

	gtk_widget_set_sensitive(ibox->all_button, !GTK_TOGGLE_BUTTON(w)->active);
}

gboolean Ignore_who_reply(struct Message *m, gpointer data)
{
	if (m)
	{
		sprintf(itmp, "%s!%s@%s", m->args[5], m->args[2], m->args[3]);
		dialog_ignore_properties(m->server, NULL, NULL, itmp);
	}

	return FALSE;
}

void dialog_ignore_properties(Server *s, struct Ignore *ign, Member *m, gchar *mask)
{
	GtkWidget *hbox, *vbox, *vbox1, *vbox2, *wtmp, *wbox, *mvbox, *ftmp;
	struct Ignore_Box *ibox = NULL;

	gboolean updating = FALSE;
	gint i;

	g_return_if_fail(s);
	g_return_if_fail(ign || mask || m);

	if (ign) updating = TRUE;
	else
	{
		gchar *c, *buf;
		gchar *nick = NULL, *user = NULL, *host = NULL;

		if (m && m->userhost)
		{
			buf = (gchar *) g_malloc(strlen(m->nick) + strlen(m->userhost) + 4);
			sprintf(buf, "%s!%s", m->nick, m->userhost);
		}
		else if (m) buf = g_strdup(m->nick);
		else buf = g_strdup(mask);

		nick = buf;

		for (c = nick; *c; c++) if (*c == '!') break;

		if (*c)
		{
			*c++ = 0;
			user = c;
		
			for (c = user; *c; c++) if (*c == '@') break;

			if (*c) { *c++ = 0; host = c; }
		}

		if (*nick && (!user || !host))
		{
			if (Server_Callback_Add(s, RPL_WHOREPLY, nick, 0, Ignore_who_reply, NULL))
			{
				VW_output(s->vw_active, T_WARNING, "sF-", s, "Retrieving %s userhost mask...", nick);
				sprintf(itmp, "WHO %s", nick);
				Server_Output(s, itmp, FALSE);
			}

			g_free(buf);
			return;
		}

		ign = (struct Ignore *) g_malloc0(sizeof(struct Ignore));
		ign->server = s;
		ign->buf = buf;
		ign->nick = nick;
		ign->user = user;
		ign->host = host;
	}

	if (ign->ignore_box) ibox = (struct Ignore_Box *) ign->ignore_box;
	else
	{
		ibox = (struct Ignore_Box *) g_malloc0(sizeof(struct Ignore_Box));
		ign->ignore_box= (gpointer) ibox;
		ibox->ignore = (struct Ignore *) ign;

		if (ign->user && ign->host)
		{
			sprintf(itmp, "%s!%s@%s", ign->nick, ign->user, ign->host);
			ibox->strings = g_list_append(ibox->strings, (gpointer) g_strdup(itmp));
			sprintf(itmp, "*!%s@%s", ign->user, ign->host);
			ibox->strings = g_list_append(ibox->strings, (gpointer) g_strdup(itmp));
			if (!(ign->Mask)) ign->Mask = g_strdup(itmp);
			sprintf(itmp, "*!%s@%s", ign->user, network_from_hostname(ign->host));
			ibox->strings = g_list_append(ibox->strings, (gpointer) g_strdup(itmp));
			sprintf(itmp, "*!*@%s", ign->host);
			ibox->strings = g_list_append(ibox->strings, (gpointer) g_strdup(itmp));
			sprintf(itmp, "*!*@%s", network_from_hostname(ign->host));
			ibox->strings = g_list_append(ibox->strings, (gpointer) g_strdup(itmp));
		}
	}

	if (ibox->dbox.window)
	{
		gdk_window_raise(ibox->dbox.window->window);
		return;
	}

	ibox->dbox.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_signal_connect((GtkObject *) ibox->dbox.window, "destroy", (GtkSignalFunc) ibox_destroy, (gpointer) ibox);
	gtk_signal_connect((GtkObject *) ibox->dbox.window, "key_press_event", (GtkSignalFunc) dialog_key_press_event, (gpointer) ibox);

	gtk_window_set_position((GtkWindow *) ibox->dbox.window, GTK_WIN_POS_CENTER);

	if (updating) sprintf(itmp, "Update an ignore mask on %s", ign->server->fs->Name);
	else sprintf(itmp, "Add an ignore mask on %s", ign->server->fs->Name);
	gtk_window_set_title((GtkWindow *) ibox->dbox.window, itmp);

	mvbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add((GtkContainer *) ibox->dbox.window, mvbox);

	ftmp = gtk_frame_new(NULL);
	gtk_container_border_width((GtkContainer *) ftmp, 2);
	gtk_box_pack_start((GtkBox *) mvbox, ftmp, TRUE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 8);
	gtk_container_border_width((GtkContainer *) vbox, 6);
   gtk_container_add((GtkContainer *) ftmp, vbox);

	wbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start((GtkBox *) vbox, wbox, FALSE, FALSE, 8);

	wtmp = gtk_label_new("Mask");
	gtk_box_pack_start((GtkBox *) wbox, wtmp, FALSE, FALSE, 8);

	ibox->combo_mask = gtk_combo_new();
	GTK_WIDGET_UNSET_FLAGS(((GtkCombo *) ibox->combo_mask)->button, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) wbox, ibox->combo_mask, TRUE, TRUE, 0);
	if (ibox->strings) gtk_combo_set_popdown_strings((GtkCombo *) ibox->combo_mask, ibox->strings);
	if (ign->Mask) gtk_entry_set_text((GtkEntry *) ((GtkCombo *) ibox->combo_mask)->entry, ign->Mask);

	hbox = gtk_hbox_new(FALSE, 8);
	gtk_box_pack_start((GtkBox *) vbox, hbox, TRUE, TRUE, 0);

	/* The left vbox */

	vbox1 = gtk_vbox_new(FALSE, 8);
	gtk_box_pack_start((GtkBox *) hbox, vbox1, TRUE, TRUE, 0);

	for (i=0; i<5; i++)
	{
		ibox->check_button[i] = gtk_check_button_new_with_label(ignore_add_check_buttons_names[i]);
		GTK_WIDGET_UNSET_FLAGS(ibox->check_button[i], GTK_CAN_FOCUS);
		gtk_box_pack_start((GtkBox *) vbox1, ibox->check_button[i], TRUE, TRUE, 0);
		if (updating)
			gtk_toggle_button_set_state(((GtkToggleButton *) &(((GtkCheckButton *) ibox->check_button[i]))->toggle_button), ign->Flags & (1<<i));
		else 
			gtk_toggle_button_set_state(((GtkToggleButton *) &(((GtkCheckButton *) ibox->check_button[i]))->toggle_button), (!i || i==3 || i==4));
	}

	/* The right vbox */

	vbox2 = gtk_vbox_new(FALSE, 8);
	gtk_box_pack_start(GTK_BOX(hbox), vbox2, TRUE, TRUE, 0);

	ibox->all_button = gtk_button_new_with_label(" Ignore all messages ");
	GTK_WIDGET_UNSET_FLAGS(ibox->all_button, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(vbox2), ibox->all_button, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(ibox->all_button), "clicked", GTK_SIGNAL_FUNC(ibox_Ignore_All), (gpointer) ibox);

	wtmp = gtk_hseparator_new();
	gtk_box_pack_start((GtkBox *) vbox2, wtmp, TRUE, TRUE, 8);

	ibox->exclude_button = gtk_toggle_button_new_with_label(" Never ignore this mask ");
	GTK_WIDGET_UNSET_FLAGS(ibox->exclude_button, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) vbox2, ibox->exclude_button, TRUE, TRUE, 0);
	gtk_signal_connect((GtkObject *) ibox->exclude_button, "clicked", (GtkSignalFunc) ibox_Exclude, (gpointer) ibox);

	gtk_toggle_button_set_state((GtkToggleButton *) ibox->exclude_button, ign->Flags & IGNORE_EXCLUDE);
	ibox_Exclude(ibox->exclude_button, (gpointer) ibox);

	wtmp = gtk_hseparator_new();
	gtk_box_pack_start((GtkBox *) vbox2, wtmp, TRUE, TRUE, 8);

	ibox->expire_on_quit_button = gtk_toggle_button_new_with_label(" Expire on server closing ");
	GTK_WIDGET_UNSET_FLAGS(ibox->expire_on_quit_button, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) vbox2, ibox->expire_on_quit_button, TRUE, TRUE, 0);
	gtk_signal_connect((GtkObject *) ibox->expire_on_quit_button, "clicked", (GtkSignalFunc) ibox_Expire_On_Quit, (gpointer) ibox);

	wbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start((GtkBox *) vbox2, wbox, TRUE, TRUE, 0);

	ibox->Label_1 = gtk_label_new("Expire after");
	gtk_box_pack_start((GtkBox *) wbox, ibox->Label_1, TRUE, TRUE, 0);

	ibox->entry_lifetime = gtk_entry_new_with_max_length(4);
	gtk_widget_set_usize(ibox->entry_lifetime, 50, -1);	/* FIXME */
	gtk_box_pack_start((GtkBox *) wbox, ibox->entry_lifetime, FALSE, FALSE, 8);
	if (!updating || !ign->expiry_date) sprintf(itmp, "10");
	else
	{
		gint32 min = (ign->expiry_date - time(NULL) + 59)/60;
		sprintf(itmp, "%d", min);
	}
	gtk_entry_set_text((GtkEntry *) ibox->entry_lifetime, itmp);

	ibox->Label_2 = gtk_label_new("mins");
	gtk_box_pack_start((GtkBox *) wbox, ibox->Label_2, TRUE, TRUE, 0);

	if (!ign->expiry_date)
	{
		gtk_toggle_button_set_state((GtkToggleButton *) ibox->expire_on_quit_button, TRUE);
		ibox_Expire_On_Quit(ibox->expire_on_quit_button, (gpointer) ibox);
	}

	/* The action area */

	hbox = gtk_hbox_new(TRUE, 0);
	gtk_container_border_width((GtkContainer *) hbox, 6);
	gtk_box_pack_end((GtkBox *) mvbox, hbox, FALSE, FALSE, 0);

	wtmp = gtk_button_new_with_label((updating)? " Update " : " Add ");
	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) hbox, wtmp, TRUE, TRUE, 16);
	gtk_signal_connect((GtkObject *) wtmp, "clicked", (GtkSignalFunc) ibox_Ok, (gpointer) ibox);

	wtmp = gtk_button_new_with_label(" Cancel ");
	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) hbox, wtmp, TRUE, TRUE, 16);
	gtk_signal_connect_object((GtkObject *) wtmp, "clicked", (GtkSignalFunc) gtk_widget_destroy, (gpointer) ibox->dbox.window);

	gtk_widget_show_all(ibox->dbox.window);

	gtk_widget_grab_focus(((GtkCombo *) ibox->combo_mask)->entry);

	if (!updating) Ignores_Not_Linked = g_list_append(Ignores_Not_Linked, (gpointer) ign);
}

/* ----- Management of Unignore Mask dialog boxes -------------------------------------- */

void dialog_unignore(Server *s, Member *m)
{
}

/* ------------------------------------------------------------------------------------- */

/* vi: set ts=3: */

