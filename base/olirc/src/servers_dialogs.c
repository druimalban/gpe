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
#include "servers_dialogs.h"
#include "prefs.h"
#include "servers.h"
#include "network.h"
#include "misc.h"
#include "windows.h"

#include <gdk/gdkkeysyms.h>

/* TODO	Les combos "networks" ne sont pas updatés en temps réel (lors de l'ajout d'un net)
 *       Les nets ne sont jamais retirés de la liste, il faudrait faire une vérification
 *       Voir s'il est plus simple d'utiliser gtk_signal_handler_block_by_func()
 *        à la place de gtk_signal_handler_block()
 *			Les comparaisons (pour vérifier si deux servers, deux nets ou autres sont égaux)
 *			 n'utilisent pas les memes routines de comparaisons partout (strcmp, g_strcasecmp..)
 *			L'ajout d'un nouveau server dans les favoris provoque un bug dans les fleches de reorder
 *			Mettre en fonction les ports des serveurs, par ex "6660-6669, 7000" (plutot dans servers.c)
 *			Rajouter un choix de network dans global_servers_list_box
 */

#define ARROW_UNSENSITIVE GTK_SHADOW_ETCHED_IN

/* -- Structures -- */

struct d_server_props_box;
struct d_favorite_server_box;
struct d_global_servers_list_box;

struct d_server_props_box
{
	struct dbox dbox;

	GtkWidget *combo_server, *combo_network;
	GtkWidget *ports, *nick, *user, *real;
	GtkWidget *location, *password;
	GtkWidget *invisible, *wallops, *motd;
	Server *server;
	Favorite_Server *fserver;
	guint32 rwid;
	gint handler_id;
	gchar *location_hidden;
	struct d_global_servers_list_box *gslb;
};

struct d_favorite_server_box
{
	struct dbox dbox;

	GtkWidget *clist;
	GtkWidget *up_button, *up_arrow;
	GtkWidget *down_button, *down_arrow;
	GtkWidget *add_button, *edit_button, *remove_button;
	gint row_selected;
	struct d_global_servers_list_box *gslb;
};

struct d_global_servers_list_box
{
	struct dbox dbox;

	GtkWidget *clist;
	GtkWidget *choose_button;
	gint Row;

	struct d_server_props_box *dspb;
	struct d_favorite_server_box *fsb;
};

/* -- Global variables -- */

static struct d_server_props_box *new_server_box = NULL;
static struct d_favorite_server_box *fs_box = NULL;

GList *Favorite_Servers_List = NULL;
GList *Favorite_Networks_List = NULL;

gchar sd_tmp[2048];

/* -- Prototypes needed -- */

void fsb_insert_server(GList *, gint);
gboolean dspb_set_servers(struct d_server_props_box *, gboolean);
void dspb_set_fields(struct d_server_props_box *, Favorite_Server *, gboolean);

void dialog_global_servers_list(struct d_global_servers_list_box *, struct d_server_props_box *, struct d_favorite_server_box *);

/* ----- Favorite Servers -------------------------------------------------------------- */

void favorite_network_add(gchar *net)
{
	/* Add a network to the favorite network list if not already present */

	GList *l;
	gchar *tmp;

	g_return_if_fail(net);
	tmp = trim_spaces(net);
	
	l = Favorite_Networks_List;
	while (l)
	{
		if (!g_strcasecmp(tmp, (gchar *) (l->data))) break;
		l = l->next;
	}
	if (l) { g_free(tmp); return; }

	Favorite_Networks_List = g_list_append(Favorite_Networks_List, tmp);

	if (new_server_box)
	{
		/* TODO Update the network list */
	}

	l = Favorite_Servers_List;

	while (l)
	{
		if (((Favorite_Server *) l->data)->properties_box)
		{
			/* TODO Update the network list */
		}
		l = l->next;
	}
}

Favorite_Server *favorite_server_find(gchar *name)
{
	/* Scan the Favorite Servers List */
	gchar *tmp;
	GList *l = Favorite_Servers_List;
	g_return_val_if_fail(name, NULL);
	tmp = strip_spaces(name);
	while (l)
	{
		if (!g_strcasecmp(((Favorite_Server *) l->data)->Name, tmp))
		{ g_free(tmp); return (Favorite_Server *) l->data; }
		l = l->next;
	}
	g_free(tmp);
	return NULL;
}

gboolean check_port_list(gchar *str)
{
	if (!str) return FALSE;
	if (!*str) return FALSE;

	if (*str < '0' || *str > '9') return FALSE;
	str++;

	while (*str)
	{
		if (*str == ' ' || *str == '\t') { str++; continue; }
		if ((*str == '-' || *str == ',') && (*(str-1) == '-' || *(str-1) == ',')) return FALSE;
		if ((*str == '-' || *str == ',')) { str++; continue; }
		if (*str < '0' || *str > '9') return FALSE;
		str++;
	}

	if (*str) return FALSE;
	if (*(str-1) == '-' || *(str-1) == ',') return FALSE;
	return TRUE;
}

Favorite_Server *favorite_server_add_or_update(gboolean alert, Favorite_Server *fs, gchar *name, gchar *ports, gchar *net, gchar *location, gchar *nick, gchar *user, gchar *real, gchar *pwd, guint32 flags)
{
	/* Add a new server to Favorite_Servers_List or update its properties if it's already in the list */

	gchar *p;
	gboolean new_server = FALSE;
	static guint32 fa_id = 0;

	g_return_val_if_fail(fs || name, NULL);
	g_return_val_if_fail(ports || location || net || pwd || nick || user || real, NULL);

	if (!fs)
	{
		fs = favorite_server_find(name);
		if (fs) name = NULL;
	}

	if (!fs && is_string_empty(name))
	{
		if (alert) Message_Box("Ol-Irc - Error", "You must specify a server name.");
		return NULL;
	}

	if (is_string_empty(nick))
	{
		if (alert)
		{
			Message_Box("Ol-Irc - Error", "You must specify a nick.");
			return NULL;
		}
		nick = "";
	}

	p = strip_spaces(ports);

	if (!check_port_list(p))
	{
		g_free(p);
		if (alert)
		{
			Message_Box("Ol-Irc - Error", "The ports are invalid.");
			return NULL;
		}
		p = "";
	}

	if (is_string_empty(real)) real = GPrefs.Default_Realname;
	if (is_string_empty(user)) user = Olirc->username;
	if (is_string_empty(location)) location = "Unknown";
	if (is_string_empty(net)) net = "Unknown";
	if (is_string_empty(pwd)) pwd = "";

	if (!fs)
	{
		fs = (Favorite_Server *) g_malloc0(sizeof(Favorite_Server));
		fs->object_id = fa_id++;
		new_server = TRUE;
	}

	if (name) g_free_and_set(fs->Name, strip_spaces(name));
	g_free_and_set(fs->Network, trim_spaces(net));
	g_free_and_set(fs->Ports, p);
	g_free_and_set(fs->Password, trim_spaces(pwd));
	g_free_and_set(fs->Nick, strip_spaces(nick));
	g_free_and_set(fs->Location, trim_spaces(location));
	g_free_and_set(fs->User, trim_spaces(user));
	g_free_and_set(fs->Real, trim_spaces(real));
	fs->Flags = flags;

	if (new_server)
	{
		fs->Row = g_list_length(Favorite_Servers_List);
		Favorite_Servers_List = g_list_append(Favorite_Servers_List, fs);
		if (fs_box) fsb_insert_server(g_list_append(NULL, fs), -1);
	}
	else if (fs_box)
	{
		GList *l = (GList *) gtk_clist_get_row_data((GtkCList *) fs_box->clist, fs->Row);
		g_return_val_if_fail(l, NULL);
		gtk_clist_freeze((GtkCList *) fs_box->clist);
		gtk_clist_remove((GtkCList *) fs_box->clist, fs->Row);
		fsb_insert_server(l, fs->Row);
		gtk_clist_select_row((GtkCList *) fs_box->clist, fs->Row, 0);
		gtk_clist_thaw((GtkCList *) fs_box->clist);
	}

	favorite_network_add(fs->Network);

	if (new_server_box)
	{
		gchar *tmp;
		tmp = gtk_entry_get_text((GtkEntry *) ((GtkCombo *) new_server_box->combo_server)->entry);
		if (is_string_empty(tmp)) 
		{
			Favorite_Server *fs_tmp = (Favorite_Server *) Favorite_Servers_List->data;
			gtk_entry_set_text((GtkEntry *) ((GtkCombo *) new_server_box->combo_server)->entry, fs_tmp->Name);
			dspb_set_fields(new_server_box, fs_tmp, TRUE);
		}
		dspb_set_servers(new_server_box, FALSE);
	}

	return fs;
}

/* ----- Favorite Servers Box ---------------------------------------------------------- */

void fsb_destroy(GtkWidget *w, gpointer data)
{
	if (fs_box->gslb) gtk_widget_destroy(fs_box->gslb->dbox.window);
	g_free_and_NULL(fs_box);
}

void fsb_insert_server(GList *l, gint row)
{
	Favorite_Server *fs;
	gchar *values[4];

	g_return_if_fail(fs_box);
	g_return_if_fail(l);
	
	fs = (Favorite_Server *) l->data;

	values[0] = fs->Name;
	values[1] = fs->Ports;
	values[2] = fs->Network;
	values[3] = fs->Location;

	if (row==-1) row = gtk_clist_append((GtkCList *) fs_box->clist, values);
	else gtk_clist_insert((GtkCList *) fs_box->clist, row, values);

	gtk_clist_set_row_data((GtkCList *) fs_box->clist, row, l);
	fs->Row = row;
}

void fsb_add(GtkWidget *w, gpointer data)
{	
	dialog_global_servers_list(fs_box->gslb, NULL, fs_box);
}

void fsb_remove(GtkWidget *w, gpointer data)
{
	GList *l;
	Favorite_Server *fs, *ftmp;
	g_return_if_fail(fs_box);
	g_return_if_fail(fs_box->row_selected != -1);
	fs = (Favorite_Server *) ((GList *) gtk_clist_get_row_data((GtkCList *) fs_box->clist, fs_box->row_selected))->data;

	l = Servers_List;

	while (l)
	{
		if (((Server *) l->data)->fs == fs) break;
		l = l->next;
	}

	if (l)
	{
		Message_Box("Ol-Irc - Error", "This server is currently being used. Please\nclose its window before trying to removing it.");
		return;
	}

	if (fs->properties_box)
	{
		gtk_widget_destroy(((struct d_server_props_box *) fs->properties_box)->dbox.window);
		sprintf(sd_tmp, "Favorite server %s has been removed.", fs->Name);
		Message_Box("Ol-Irc", sd_tmp);
	}

	Favorite_Servers_List = g_list_remove(Favorite_Servers_List, fs);
	
	l = Favorite_Servers_List;
	while (l)
	{
		ftmp = (Favorite_Server *) l->data;
		if (ftmp->Row >= fs->Row) ftmp->Row--;
		l = l->next;
	}

	if (fs_box) gtk_clist_remove((GtkCList *) fs_box->clist, fs->Row);

	if (new_server_box) dspb_set_servers(new_server_box, FALSE);
}

void fsb_edit(GtkWidget *w, gpointer data)
{
	GList *l;
	g_return_if_fail(fs_box);
	g_return_if_fail(fs_box->row_selected != -1);
	l = (GList *) gtk_clist_get_row_data((GtkCList *) fs_box->clist, fs_box->row_selected);
	dialog_server_properties((struct Favorite_Server *) l->data, NULL, NULL);
}

void fsb_reorder(GtkWidget *w, gpointer data)
{
	GList *l;
	Favorite_Server *fs;
	gint row = fs_box->row_selected;

	g_return_if_fail(row != -1);

	l = (GList *) gtk_clist_get_row_data((GtkCList *) fs_box->clist, row);

	fs = (Favorite_Server *) l->data;

	gtk_clist_freeze((GtkCList *) fs_box->clist);
	gtk_clist_remove((GtkCList *) fs_box->clist, row);

	if (data)
	{
		l->data = l->prev->data;
		l->prev->data = fs;
		gtk_clist_set_row_data((GtkCList *) fs_box->clist, (((Favorite_Server *) l->data)->Row)++, l);
		l = l->prev;
		row--;
	}
	else
	{	
		l->data = l->next->data;
		l->next->data = fs;
		gtk_clist_set_row_data((GtkCList *) fs_box->clist, --(((Favorite_Server *) l->data)->Row), l);
		l = l->next;
		row++;
	}

	fsb_insert_server(l, row);
	gtk_clist_select_row((GtkCList *) fs_box->clist, row, 0);
	gtk_clist_thaw((GtkCList *) fs_box->clist);
}

void fsb_unselect_row(GtkWidget *w, gint row, gint column, GdkEventButton *v, gpointer data)
{
	/* A row has been unselected in the clist */

	gtk_widget_set_sensitive(fs_box->remove_button, FALSE);
	gtk_widget_set_sensitive(fs_box->edit_button, FALSE);
	gtk_widget_set_sensitive(fs_box->up_button, FALSE);
	gtk_arrow_set((GtkArrow *) fs_box->up_arrow, GTK_ARROW_UP, ARROW_UNSENSITIVE);
	gtk_widget_set_sensitive(fs_box->down_button, FALSE);
	gtk_arrow_set((GtkArrow *) fs_box->down_arrow, GTK_ARROW_DOWN, ARROW_UNSENSITIVE);
	fs_box->row_selected = -1;
}

void fsb_select_row(GtkWidget *w, gint row, gint column, GdkEventButton *v, gpointer data)
{
	/* A row has been selected in the clist */

	GList *l = (GList *) gtk_clist_get_row_data((GtkCList *) fs_box->clist, row);

	gtk_widget_set_sensitive(fs_box->remove_button, TRUE);
	gtk_widget_set_sensitive(fs_box->edit_button, TRUE);

	if (l->prev)
	{
		gtk_widget_set_sensitive(fs_box->up_button, TRUE);
		gtk_arrow_set((GtkArrow *) fs_box->up_arrow, GTK_ARROW_UP, GTK_SHADOW_IN);
	}
	else
	{
		gtk_widget_set_sensitive(fs_box->up_button, FALSE);
		gtk_arrow_set((GtkArrow *) fs_box->up_arrow, GTK_ARROW_UP, ARROW_UNSENSITIVE);
	}

	if (l->next)
	{
		gtk_widget_set_sensitive(fs_box->down_button, TRUE);
		gtk_arrow_set((GtkArrow *) fs_box->down_arrow, GTK_ARROW_DOWN, GTK_SHADOW_IN);
	}
	else
	{
		gtk_widget_set_sensitive(fs_box->down_button, FALSE);
		gtk_arrow_set((GtkArrow *) fs_box->down_arrow, GTK_ARROW_DOWN, ARROW_UNSENSITIVE);
	}

	fs_box->row_selected = row;
}

static gchar *servers_clist_titles[] = { "Server", "Ports", "Network", "Location" };

void dialog_favorite_servers()
{
	GtkWidget *vbox, *hbox, *ftmp;
	GtkWidget *wtmp, *btmp;
	GList *l;

	if (fs_box)
	{
		gdk_window_raise(fs_box->dbox.window->window);
		return;
	}

	fs_box = (struct d_favorite_server_box *) g_malloc0(sizeof(struct d_favorite_server_box));

	fs_box->dbox.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	fs_box->row_selected = -1;

	gtk_window_set_position((GtkWindow *) fs_box->dbox.window, GTK_WIN_POS_CENTER);
	gtk_signal_connect((GtkObject *) fs_box->dbox.window, "destroy", (GtkSignalFunc) fsb_destroy, NULL);
	gtk_signal_connect((GtkObject *) fs_box->dbox.window, "key_press_event", (GtkSignalFunc) dialog_key_press_event, (gpointer) fs_box);
	gtk_widget_set_usize(fs_box->dbox.window, 610, 300);	/* FIXME */
	gtk_window_set_title((GtkWindow *) fs_box->dbox.window, "Your favorite IRC servers");

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add((GtkContainer *) fs_box->dbox.window, vbox);

	ftmp = gtk_frame_new(NULL);
	gtk_container_border_width((GtkContainer *) ftmp, 2);
	gtk_box_pack_start((GtkBox *) vbox, ftmp, TRUE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_border_width((GtkContainer *) hbox, 6);
   gtk_container_add((GtkContainer *) ftmp, hbox);

	fs_box->clist = gtk_clist_new_with_titles(4, servers_clist_titles);
	gtk_signal_connect((GtkObject *) fs_box->clist, "select_row", (GtkSignalFunc) fsb_select_row, NULL);
	gtk_signal_connect((GtkObject *) fs_box->clist, "unselect_row", (GtkSignalFunc) fsb_unselect_row, NULL);
	gtk_clist_column_titles_passive((GtkCList *) fs_box->clist);
	gtk_clist_set_column_width((GtkCList *) fs_box->clist, 0, 160);
	gtk_clist_set_column_width((GtkCList *) fs_box->clist, 1, 130);
	gtk_clist_set_column_width((GtkCList *) fs_box->clist, 2, 100);
	gtk_clist_set_column_width((GtkCList *) fs_box->clist, 3, 100);

	{
		GtkWidget *wtmp = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy((GtkScrolledWindow *) wtmp, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		GTK_WIDGET_UNSET_FLAGS(((GtkScrolledWindow *) wtmp)->vscrollbar, GTK_CAN_FOCUS);
		GTK_WIDGET_UNSET_FLAGS(fs_box->clist, GTK_CAN_FOCUS);
		gtk_container_add((GtkContainer *) wtmp, fs_box->clist);
		gtk_box_pack_start((GtkBox *) hbox, wtmp, TRUE, TRUE, 0);
	}

	l = Favorite_Servers_List;
	while (l) { fsb_insert_server(l, -1); l = l->next; }

	btmp = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start((GtkBox *) hbox, btmp, FALSE, FALSE, 0);

	fs_box->up_button = gtk_button_new();
	gtk_widget_set_sensitive(fs_box->up_button, FALSE);
	gtk_signal_connect((GtkObject *) fs_box->up_button, "clicked", (GtkSignalFunc) fsb_reorder, (gpointer) TRUE);
	GTK_WIDGET_UNSET_FLAGS(fs_box->up_button, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, fs_box->up_button, TRUE, TRUE, 0);
	fs_box->up_arrow = gtk_arrow_new(GTK_ARROW_UP, ARROW_UNSENSITIVE);
	gtk_container_add((GtkContainer *) fs_box->up_button, fs_box->up_arrow);

	fs_box->down_button = gtk_button_new();
	gtk_widget_set_sensitive(fs_box->down_button, FALSE);
	gtk_signal_connect((GtkObject *) fs_box->down_button, "clicked", (GtkSignalFunc) fsb_reorder, (gpointer) FALSE);
	GTK_WIDGET_UNSET_FLAGS(fs_box->down_button, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, fs_box->down_button, TRUE, TRUE, 0);
	fs_box->down_arrow = gtk_arrow_new(GTK_ARROW_DOWN, ARROW_UNSENSITIVE);
	gtk_container_add((GtkContainer *) fs_box->down_button, fs_box->down_arrow);

	btmp = gtk_hbox_new(TRUE, 10);
	gtk_container_border_width((GtkContainer *) btmp, 10);
	gtk_box_pack_start((GtkBox *) vbox, btmp, FALSE, FALSE, 0);

	fs_box->add_button = gtk_button_new_with_label(" Add a server ");
	GTK_WIDGET_UNSET_FLAGS(fs_box->add_button, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, fs_box->add_button, TRUE, TRUE, 0);
	gtk_signal_connect((GtkObject *) fs_box->add_button, "clicked", (GtkSignalFunc) fsb_add, NULL);

	fs_box->remove_button = gtk_button_new_with_label(" Remove this server ");
	GTK_WIDGET_UNSET_FLAGS(fs_box->remove_button, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, fs_box->remove_button, TRUE, TRUE, 0);
	gtk_widget_set_sensitive(fs_box->remove_button, FALSE);
	gtk_signal_connect((GtkObject *) fs_box->remove_button, "clicked", (GtkSignalFunc) fsb_remove, NULL);

	fs_box->edit_button = gtk_button_new_with_label(" Edit this server ");
	GTK_WIDGET_UNSET_FLAGS(fs_box->edit_button, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, fs_box->edit_button, TRUE, TRUE, 0);
	gtk_widget_set_sensitive(fs_box->edit_button, FALSE);
	gtk_signal_connect((GtkObject *) fs_box->edit_button, "clicked", (GtkSignalFunc) fsb_edit, NULL);

	wtmp = gtk_button_new_with_label(" Close this window ");
	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, wtmp, TRUE, TRUE, 0);
	gtk_signal_connect_object((GtkObject *) wtmp, "clicked", (GtkSignalFunc) gtk_widget_destroy, (gpointer) fs_box->dbox.window);

	gtk_widget_show_all(fs_box->dbox.window);
}

/* ----- Server properties box (and choose new server box) ----------------------------- */

void dspb_set_fields(struct d_server_props_box *dspb, Favorite_Server *fs, gboolean change_network)
{
	g_return_if_fail(dspb);
	g_return_if_fail(fs);

	if (change_network) gtk_entry_set_text((GtkEntry *) ((GtkCombo *) dspb->combo_network)->entry, fs->Network);
	gtk_entry_set_text((GtkEntry *) dspb->ports, fs->Ports);
	if (dspb->location) gtk_entry_set_text((GtkEntry *) dspb->location, fs->Location);
	gtk_entry_set_text((GtkEntry *) dspb->nick, fs->Nick);
	gtk_entry_set_text((GtkEntry *) dspb->user, fs->User);
	gtk_entry_set_text((GtkEntry *) dspb->real, fs->Real);
	gtk_entry_set_text((GtkEntry *) dspb->password, fs->Password);
	gtk_toggle_button_set_state((GtkToggleButton *) &((GtkCheckButton *) dspb->invisible)->toggle_button, fs->Flags & FSERVER_INVISIBLE);
	gtk_toggle_button_set_state((GtkToggleButton *) &((GtkCheckButton *) dspb->wallops)->toggle_button, fs->Flags & FSERVER_WALLOPS);
	gtk_toggle_button_set_state((GtkToggleButton *) &((GtkCheckButton *) dspb->motd)->toggle_button, fs->Flags & FSERVER_HIDEMOTD);
}

void dspb_server_selected(GtkWidget *w, gpointer data)
{
	struct d_server_props_box *dspb = (struct d_server_props_box *) data;
	Favorite_Server *fs;
	gchar *tmp;

	g_return_if_fail(data);

	if (!(((GtkList *) w)->selection)) return;

	fs = favorite_server_find(gtk_entry_get_text((GtkEntry *) ((GtkCombo *) dspb->combo_server)->entry));

	if (!fs) return;

	tmp = gtk_entry_get_text((GtkEntry *) ((GtkCombo *) dspb->combo_network)->entry);

	dspb_set_fields(dspb, fs, is_string_empty(tmp));
}

gboolean dspb_set_servers(struct d_server_props_box *dspb, gboolean update)
{
	GList *l, *s;
	gchar *tmp;
	gboolean ret = TRUE;

	g_return_val_if_fail(dspb, FALSE);

	tmp = gtk_entry_get_text((GtkEntry *) ((GtkCombo *) dspb->combo_network)->entry);

	s = NULL; l = Favorite_Servers_List;

	while (l)
	{
		if (!g_strcasecmp(((Favorite_Server *) l->data)->Network, tmp))
			s = g_list_append(s, ((Favorite_Server *) l->data)->Name);
		l = l->next;
	}

	if (!s)
	{
		l = Favorite_Servers_List;
		while (l)
		{
			s = g_list_append(s, ((Favorite_Server *) l->data)->Name);
			l = l->next;
		}
	}

	if (!s)
	{
		s = g_list_append(NULL, "");
		ret = FALSE;
	}

	if (!update)
	{	
 		tmp = g_strdup(gtk_entry_get_text((GtkEntry *) ((GtkCombo *) dspb->combo_server)->entry));
		gtk_signal_handler_block((GtkObject *) ((GtkCombo *) dspb->combo_server)->list, dspb->handler_id);
	}

	gtk_combo_set_popdown_strings((GtkCombo *) dspb->combo_server, s);

	if (!update)
	{
		gtk_entry_set_text((GtkEntry *) ((GtkCombo *) dspb->combo_server)->entry, tmp);
		g_free(tmp);
		gtk_signal_handler_unblock((GtkObject *) ((GtkCombo *) dspb->combo_server)->list, dspb->handler_id);
	}

	return ret;
}

void dspb_network_selected(GtkWidget *w, gpointer data)
{
	g_return_if_fail(data);
	dspb_set_servers((struct d_server_props_box *) data, FALSE);
}

void dspb_edit_button(GtkWidget *w, gpointer data)
{
	dialog_favorite_servers();
}

void dspb_destroy(GtkWidget *w, gpointer data)
{
	struct d_server_props_box *dspb = (struct d_server_props_box *) data;

	g_return_if_fail(data);

	if (dspb->gslb) gtk_widget_destroy(dspb->gslb->dbox.window);

	if (new_server_box == dspb)
		g_free_and_NULL(new_server_box);
	else if (dspb->fserver && dspb->fserver->properties_box == dspb)
		g_free_and_NULL(dspb->fserver->properties_box);
}

Favorite_Server *dspb_store_server(struct d_server_props_box *dspb, gboolean update)
{
	Favorite_Server *fs;
	gint f = 0;

	g_return_val_if_fail(dspb, NULL);

	if (((GtkToggleButton *) &((GtkCheckButton *) dspb->invisible)->toggle_button)->active) f |= FSERVER_INVISIBLE;
	if (((GtkToggleButton *) &((GtkCheckButton *) dspb->wallops)->toggle_button)->active) f |= FSERVER_WALLOPS;
	if (((GtkToggleButton *) &((GtkCheckButton *) dspb->motd)->toggle_button)->active) f |= FSERVER_HIDEMOTD;

	fs = favorite_server_add_or_update(
		TRUE,
		(dspb->fserver)? dspb->fserver : NULL,
		(dspb->fserver)? NULL : gtk_entry_get_text((GtkEntry *) ((GtkCombo *) dspb->combo_server)->entry),
		gtk_entry_get_text((GtkEntry *) dspb->ports),
		gtk_entry_get_text((GtkEntry *) ((GtkCombo *) dspb->combo_network)->entry),
		(dspb->location)? gtk_entry_get_text((GtkEntry *) dspb->location) : 
			(dspb->location_hidden)? dspb->location_hidden : NULL,
		gtk_entry_get_text((GtkEntry *) dspb->nick),
		gtk_entry_get_text((GtkEntry *) dspb->user),
		gtk_entry_get_text((GtkEntry *) dspb->real),
		gtk_entry_get_text((GtkEntry *) dspb->password),
		f);

	if (update && fs && fs->properties_box) dspb_set_fields(fs->properties_box, fs, TRUE);

	return fs;
}

void dspb_connect_now(GtkWidget *w, gpointer data)
{
	struct d_server_props_box *dspb = (struct d_server_props_box *) data;
	Favorite_Server *fs;
	Server *s;
	g_return_if_fail(data);
	fs = dspb_store_server(dspb, TRUE);
	if (!fs) return;

	s = Server_New(fs, TRUE, GW_find_by_id(dspb->rwid));
	if (s)
	{
		gtk_widget_destroy(dspb->dbox.window);
		Server_Connect(s);
	}
}

void dspb_return_func_connect_now(gpointer data)
{
	g_return_if_fail(data);
	dspb_connect_now(NULL, data);
}

void dspb_connect_later(GtkWidget *w, gpointer data)
{
	struct d_server_props_box *dspb = (struct d_server_props_box *) data;
	Favorite_Server *fs;
	Server *s;
	g_return_if_fail(data);
	fs = dspb_store_server(dspb, TRUE);
	if (!fs) return;
	s = Server_New(fs, TRUE, GW_find_by_id(dspb->rwid));
	VW_output(s->vw, T_WARNING, "#st", s, "\nTo connect, right-click on the server's tab, then choose 'Server->Connect'.\n");
	if (s) gtk_widget_destroy(dspb->dbox.window);
}

void dspb_save_button(GtkWidget *w, gpointer data)
{
	struct d_server_props_box *dspb = (struct d_server_props_box *) data;
	Favorite_Server *fs;
	g_return_if_fail(data);
	fs = dspb_store_server(dspb, FALSE);

	if (!fs) return;

	if (new_server_box && (!g_strcasecmp(gtk_entry_get_text((GtkEntry *) ((GtkCombo *) new_server_box->combo_server)->entry), dspb->fserver->Name)))
		dspb_set_fields(new_server_box, dspb->fserver, TRUE);

	gtk_widget_destroy(dspb->dbox.window);
}

void dspb_choose_from_list(GtkWidget *w, gpointer data)
{
	dialog_global_servers_list(new_server_box->gslb, new_server_box, NULL);
}

void dspb_return_func_save(gpointer data)
{
	g_return_if_fail(data);
	dspb_save_button(NULL, data);
}

void dialog_server_properties(Favorite_Server *fs, Server *s, GUI_Window *rw)
{
	GtkWidget *wtmp, *ftmp, *btmp, *vbox;
	GtkWidget *table;
	struct d_server_props_box *dspb;
	gint row, net_row = 1, srv_row = 0;

	g_return_if_fail(!fs || !s);

	if (fs && fs->properties_box)
	{
		gdk_window_raise(((struct d_server_props_box *) fs->properties_box)->dbox.window->window);
		return;
	}
	else if (s && s->fs->properties_box)
	{
		gdk_window_raise(((struct d_server_props_box *) s->fs->properties_box)->dbox.window->window);
		return;
	}
	else if (!fs && !s && new_server_box)
	{
		gdk_window_raise(new_server_box->dbox.window->window);
		return;
	}

	dspb = (struct d_server_props_box *) g_malloc0(sizeof(struct d_server_props_box));

	if (rw) dspb->rwid = rw->object_id;

	if (s) { dspb->server = s; fs = s->fs; }
	if (fs) { dspb->fserver = fs; fs->properties_box = (gpointer) dspb; }
	if (!s && !fs) new_server_box = dspb;

	/* Dialog window creation */

	dspb->dbox.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_signal_connect((GtkObject *) dspb->dbox.window, "destroy", (GtkSignalFunc) dspb_destroy, (gpointer) dspb);
	gtk_signal_connect((GtkObject *) dspb->dbox.window, "key_press_event", (GtkSignalFunc) dialog_key_press_event, (gpointer) dspb);

	if (fs) sprintf(sd_tmp, "Properties of server %s", (s)? s->fs->Name : fs->Name);
	else sprintf(sd_tmp, "Please choose an IRC server");
		
	gtk_window_set_title((GtkWindow *) dspb->dbox.window, sd_tmp);
	gtk_window_set_position((GtkWindow *) dspb->dbox.window, GTK_WIN_POS_CENTER);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add((GtkContainer *) dspb->dbox.window, vbox);

	ftmp = gtk_frame_new(NULL);
	gtk_container_border_width((GtkContainer *) ftmp, 2);
	gtk_box_pack_start((GtkBox *) vbox, ftmp, TRUE, TRUE, 0);

	btmp = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width((GtkContainer *) btmp, 6);
   gtk_container_add((GtkContainer *) ftmp, btmp);

	table = gtk_table_new((fs)? 8 : 7, 5, FALSE);
	gtk_table_set_row_spacings((GtkTable *) table, 10);

	/* ---- The left part : Combos and Entries ---- */

	row = 0;

	if (!fs) { net_row = 0; srv_row = 1; }
	else { net_row = 1; srv_row = 0; }

	wtmp = gtk_label_new("Server");
	gtk_table_attach((GtkTable *) table, wtmp, 0, 1, srv_row, srv_row+1, (GtkAttachOptions) 0, (GtkAttachOptions) 0, 10, 0);

	if (!fs)
	{
		dspb->combo_server = gtk_combo_new();
		gtk_combo_set_case_sensitive((GtkCombo *) dspb->combo_server, FALSE);
		gtk_combo_set_use_arrows_always((GtkCombo *) dspb->combo_server, TRUE);
		GTK_WIDGET_UNSET_FLAGS(((GtkCombo *) dspb->combo_server)->button, GTK_CAN_FOCUS);
		gtk_table_attach_defaults((GtkTable *) table, dspb->combo_server, 1, 2, srv_row, srv_row+1);
		dspb->handler_id = gtk_signal_connect((GtkObject *) ((GtkCombo *) dspb->combo_server)->list, "selection_changed", (GtkSignalFunc) dspb_server_selected, (gpointer) dspb);
	}
	else
	{
		wtmp = gtk_entry_new();
		gtk_entry_set_text((GtkEntry *) wtmp, fs->Name);
		gtk_widget_set_sensitive(wtmp, FALSE);
		gtk_table_attach_defaults((GtkTable *) table, wtmp, 1, 2, srv_row, srv_row+1);
	}

	row++;

	wtmp = gtk_label_new("Network");
	gtk_table_attach((GtkTable *) table, wtmp, 0, 1, net_row, net_row+1, (GtkAttachOptions) 0, (GtkAttachOptions) 0, 10, 0);
	dspb->combo_network = gtk_combo_new();
	gtk_combo_set_case_sensitive((GtkCombo *) dspb->combo_network, FALSE);
	gtk_combo_set_use_arrows_always((GtkCombo *) dspb->combo_network, TRUE);
	GTK_WIDGET_UNSET_FLAGS(((GtkCombo *) dspb->combo_network)->button, GTK_CAN_FOCUS);
	gtk_table_attach_defaults((GtkTable *) table, dspb->combo_network, 1, 2, net_row, net_row+1);
	if (Favorite_Networks_List) gtk_combo_set_popdown_strings((GtkCombo *) dspb->combo_network, Favorite_Networks_List);
	if (fs && fs->Network) gtk_entry_set_text((GtkEntry *) ((GtkCombo *) dspb->combo_network)->entry, fs->Network);
	if (!fs) gtk_signal_connect_while_alive((GtkObject *) ((GtkCombo *) dspb->combo_network)->list, "selection_changed", (GtkSignalFunc) dspb_network_selected, (gpointer) dspb, (GtkObject *) dspb->dbox.window);
	row++;

	dspb->ports = Add_Dialog_Table_Entry(table, "Ports", 0, row++);
	if (fs) dspb->location = Add_Dialog_Table_Entry(table, "Location", 0, row++);
	dspb->nick = Add_Dialog_Table_Entry(table, "Nick", 0, row++);
	dspb->user = Add_Dialog_Table_Entry(table, "User name", 0, row++);
	dspb->real = Add_Dialog_Table_Entry(table, "Real name", 0, row++);
	dspb->password = Add_Dialog_Table_Entry(table, "Password", 0, row++);

	/* ---- The right part : Push Buttons and Check Buttons ---- */

	if (!fs)
	{
		wtmp = gtk_button_new_with_label("Choose from list");
		GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
		gtk_signal_connect((GtkObject *) wtmp, "clicked", (GtkSignalFunc) dspb_choose_from_list, (gpointer) dspb);
		gtk_table_attach((GtkTable *) table, wtmp, 2, 3, 0, 1, (GtkAttachOptions) (GTK_FILL | GTK_EXPAND), (GtkAttachOptions) 0, 10, 0);

		wtmp = gtk_button_new_with_label(" Edit favorite servers ");
		GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
		gtk_signal_connect((GtkObject *) wtmp, "clicked", (GtkSignalFunc) dspb_edit_button, NULL);
		gtk_table_attach((GtkTable *) table, wtmp, 2, 3, 1, 2, (GtkAttachOptions) (GTK_FILL | GTK_EXPAND), (GtkAttachOptions) 0, 10, 0);
	}

	dspb->invisible = gtk_check_button_new_with_label("Invisible");
	GTK_WIDGET_UNSET_FLAGS(dspb->invisible, GTK_CAN_FOCUS);
	gtk_table_attach((GtkTable *) table, dspb->invisible, 2, 3, row-4, row-3, (GtkAttachOptions) (GTK_FILL | GTK_EXPAND), (GtkAttachOptions) 0, 10, 0);

	dspb->wallops = gtk_check_button_new_with_label("Wallops");
	GTK_WIDGET_UNSET_FLAGS(dspb->wallops, GTK_CAN_FOCUS);
	gtk_table_attach((GtkTable *) table, dspb->wallops, 2, 3, row-3, row-2, (GtkAttachOptions) (GTK_FILL | GTK_EXPAND), (GtkAttachOptions) 0, 10, 0);

	dspb->motd = gtk_check_button_new_with_label("Hide MOTD");
	GTK_WIDGET_UNSET_FLAGS(dspb->motd, GTK_CAN_FOCUS);
	gtk_table_attach((GtkTable *) table, dspb->motd, 2, 3, row-2, row-1, (GtkAttachOptions) (GTK_FILL | GTK_EXPAND), (GtkAttachOptions) 0, 10, 0);

	gtk_box_pack_start((GtkBox *) (GtkDialog *) btmp, table, TRUE, TRUE, 0);

	if (!fs)
	{
		if (!dspb_set_servers(dspb, TRUE))
		{
			gtk_toggle_button_set_state((GtkToggleButton *) &((GtkCheckButton *) dspb->invisible)->toggle_button, TRUE);
			gtk_entry_set_text((GtkEntry *) dspb->ports, "6667");
			gtk_entry_set_text((GtkEntry *) dspb->user, Olirc->username);
			gtk_entry_set_text((GtkEntry *) dspb->real, GPrefs.Default_Realname);
		}

		gtk_widget_grab_focus(((GtkCombo *) dspb->combo_server)->entry);
	}
	else dspb_set_fields(dspb, fs, FALSE);

	/* ---- Action buttons ---- */

	btmp = gtk_hbox_new(TRUE, 0);
	gtk_container_border_width((GtkContainer *) btmp, 6);
	gtk_box_pack_end((GtkBox *) vbox, btmp, FALSE, FALSE, 0);

	if (fs)
	{
		wtmp = gtk_button_new_with_label(" Save properties ");
		gtk_signal_connect((GtkObject *) wtmp, "clicked", (GtkSignalFunc) dspb_save_button, (gpointer) dspb);
		dspb->dbox.return_func = dspb_return_func_save;
	}
	else
	{
		wtmp = gtk_button_new_with_label(" Open and connect ");
		GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
		gtk_signal_connect((GtkObject *) wtmp, "clicked", (GtkSignalFunc) dspb_connect_now, (gpointer) dspb);
		gtk_box_pack_start((GtkBox *) btmp, wtmp, TRUE, TRUE, 10);
		dspb->dbox.return_func = dspb_return_func_connect_now;

		wtmp = gtk_button_new_with_label(" Open only ");
		gtk_signal_connect((GtkObject *) wtmp, "clicked", (GtkSignalFunc) dspb_connect_later, (gpointer) dspb);
	}

	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, wtmp, TRUE, TRUE, 10);

	wtmp = gtk_button_new_with_label(" Cancel ");
	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_signal_connect_object((GtkObject *) wtmp, "clicked", (GtkSignalFunc) gtk_widget_destroy, (gpointer) dspb->dbox.window);
	gtk_box_pack_start((GtkBox *) btmp, wtmp, TRUE, TRUE, 10);

	gtk_widget_show_all(dspb->dbox.window);
}

/* ----- Nickname dialog box ---------------------------------------------------------- */

struct d_server_nick_box
{
	struct dbox dbox;
	GtkWidget *entry;
	Server *server;
	gboolean dont_disconnect;
};

void snb_destroy(GtkWidget *w, gpointer data)
{
	struct d_server_nick_box *snb = (struct d_server_nick_box *) data;
	if (!snb->dont_disconnect && !snb->server->current_nick && snb->server->State != SERVER_IDLE)
		Server_Disconnect(snb->server, TRUE);
	g_free_and_NULL(snb->server->nick_box); /* snb == snb->server->nick_box, but snb->server->nick_box must be set to NULL */
}

void snb_return_func(gpointer data)
{
	gchar *nick;
	struct d_server_nick_box *snb = (struct d_server_nick_box *) data;
	g_return_if_fail(snb);

	nick = gtk_entry_get_text((GtkEntry *) snb->entry);

	if (*nick)
	{
		gchar tmp[512];
		sprintf(tmp,"NICK %s", nick);
		Server_Output(snb->server, tmp, TRUE);
		snb->dont_disconnect = TRUE;
	}

	gtk_widget_destroy(snb->dbox.window);
}

void dialog_server_nick(Server *s, gchar *prompt, gchar *nick)
{
	struct d_server_nick_box *snb;

	GtkWidget *wtmp, *btmp, *vbox, *ftmp;

	g_return_if_fail(s);

	if (s->nick_box)
	{
		gdk_window_raise(((struct d_server_nick_box *) s->nick_box)->dbox.window->window);
		return;
	}

	if (s->State!=SERVER_CONNECTED)
	{
		VW_output(s->vw_active, T_WARNING, "st", s, "Server not connected.");
		return;
	}

	if (!nick)
	{
		if (s->current_nick) nick = s->current_nick;
		else nick = s->fs->Nick;
	}

	snb = (struct d_server_nick_box *) g_malloc0(sizeof(struct d_server_nick_box));

	snb->dbox.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	snb->dbox.stop_focus = TRUE;
	snb->dbox.return_func = snb_return_func;

	snb->server = s;
	s->nick_box = (gpointer) snb;

	gtk_window_set_title((GtkWindow *) snb->dbox.window, s->fs->Name);
	gtk_window_set_position((GtkWindow *) snb->dbox.window, GTK_WIN_POS_MOUSE);

	gtk_signal_connect((GtkObject *) snb->dbox.window, "key_press_event", (GtkSignalFunc) dialog_key_press_event, (gpointer) snb);
	gtk_signal_connect((GtkObject *) snb->dbox.window, "destroy", (GtkSignalFunc) snb_destroy, (gpointer) snb);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add((GtkContainer *) snb->dbox.window, vbox);

	ftmp = gtk_frame_new(NULL);
	gtk_container_border_width((GtkContainer *) ftmp, 2);
	gtk_box_pack_start((GtkBox *) vbox, ftmp, TRUE, TRUE, 0);

	btmp = gtk_vbox_new(FALSE, 10);
	gtk_container_add((GtkContainer *) ftmp, btmp);
	gtk_container_border_width((GtkContainer *) btmp, 15);

	wtmp = gtk_label_new(prompt);
	gtk_box_pack_start((GtkBox *) btmp, wtmp, TRUE, TRUE, 0);

	snb->entry = gtk_entry_new();
	gtk_entry_set_text((GtkEntry *) snb->entry, nick);
	gtk_entry_select_region((GtkEntry *) snb->entry, 0, strlen(nick));
	gtk_box_pack_start((GtkBox *) btmp, snb->entry, TRUE, TRUE, 0);

	btmp = gtk_hbox_new(TRUE, 0);
	gtk_container_border_width((GtkContainer *) btmp, 6);
	gtk_box_pack_end((GtkBox *) vbox, btmp, FALSE, FALSE, 0);

	wtmp = gtk_button_new_with_label(" Ok ");
	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, wtmp, TRUE, TRUE, 10);
	gtk_signal_connect_object((GtkObject *) wtmp, "clicked", (GtkSignalFunc) snb_return_func, (gpointer) snb);

	wtmp = gtk_button_new_with_label(" Cancel ");
	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, wtmp, TRUE, TRUE, 10);
	gtk_signal_connect_object((GtkObject *) wtmp, "clicked", (GtkSignalFunc) gtk_widget_destroy, (gpointer) snb->dbox.window);

	gtk_widget_grab_focus(snb->entry);
	gtk_widget_show_all(snb->dbox.window);
}

/* ----- Global servers list dialog box ----------------------------------------------- */

void gslb_add_or_choose(GtkWidget *w, gpointer data)
{
	struct d_global_servers_list_box *gslb = (struct d_global_servers_list_box *) data;
	struct Global_Server *gs;

	g_return_if_fail(gslb->Row != -1);

	gs = (struct Global_Server *) gtk_clist_get_row_data((GtkCList *) gslb->clist, gslb->Row);

	if (gslb->dspb)
	{
		gtk_entry_set_text((GtkEntry *) ((GtkCombo *) gslb->dspb->combo_server)->entry, gs->name);
		gtk_entry_set_text((GtkEntry *) ((GtkCombo *) gslb->dspb->combo_network)->entry, gs->network);
		gtk_entry_set_text((GtkEntry *) gslb->dspb->ports, gs->ports);
		gtk_toggle_button_set_state((GtkToggleButton *) &((GtkCheckButton *) gslb->dspb->invisible)->toggle_button, TRUE);
		gtk_toggle_button_set_state((GtkToggleButton *) &((GtkCheckButton *) gslb->dspb->wallops)->toggle_button, FALSE);
		gtk_toggle_button_set_state((GtkToggleButton *) &((GtkCheckButton *) gslb->dspb->motd)->toggle_button, FALSE);
		gslb->dspb->location_hidden = gs->location;
	}
	else if (gslb->fsb)
	{
		favorite_server_add_or_update(FALSE, NULL, gs->name, gs->ports, gs->network, gs->location, NULL, NULL, NULL, NULL, FSERVER_INVISIBLE);
	}

	gtk_widget_destroy(gslb->dbox.window);
}

void gslb_unselect_row(GtkWidget *w, gint row, gint column, GdkEventButton *v, gpointer data)
{
	struct d_global_servers_list_box *gslb = (struct d_global_servers_list_box *) data;
	gslb->Row = -1;
	gtk_widget_set_sensitive(gslb->choose_button, FALSE);
}

void gslb_select_row(GtkWidget *w, gint row, gint column, GdkEventButton *v, gpointer data)
{
	struct d_global_servers_list_box *gslb = (struct d_global_servers_list_box *) data;
	gslb->Row = row;
	gtk_widget_set_sensitive(gslb->choose_button, TRUE);
}

gint gslb_button_press_event(GtkWidget *w, GdkEventButton *e, gpointer data)
{
	struct d_global_servers_list_box *gslb = (struct d_global_servers_list_box *) data;
	gint col, row;

	if (e->button != 1 || (e->type != GDK_2BUTTON_PRESS && e->type != GDK_3BUTTON_PRESS))
		return FALSE;

	if (gslb->Row == -1)
	{
		if (!gtk_clist_get_selection_info((GtkCList *) gslb->clist, e->x, e->y, &row, &col)) return FALSE;
		gslb->Row = row;
	}

	gslb_add_or_choose(w, data);

	return TRUE;
}

void gslb_destroy(GtkWidget *w, gpointer data)
{
	struct d_global_servers_list_box *gslb = (struct d_global_servers_list_box *) data;
	if (gslb->dspb) gslb->dspb->gslb = NULL;
	if (gslb->fsb) gslb->fsb->gslb = NULL;
	g_free(gslb);
}

void gslb_fill_list(struct d_global_servers_list_box *gslb, gchar *network)
{
	GList *l;
	gchar *values[4];
	struct Global_Server *gs;
	gint row;

	gtk_clist_clear((GtkCList *) gslb->clist);

	l = Global_Servers_List;

	while (l)
	{
		gs = (struct Global_Server *) l->data;
		values[0] = gs->name;
		values[1] = gs->ports;
		values[2] = gs->network;
		values[3] = gs->location;
		row = gtk_clist_append((GtkCList *) gslb->clist, values);
		gtk_clist_set_row_data((GtkCList *) gslb->clist, row, (gpointer) gs);
		l = l->next;
	}
}

void dialog_global_servers_list(struct d_global_servers_list_box *gslb, struct d_server_props_box *dspb, struct d_favorite_server_box *fsb)
{
	GtkWidget *vbox, *ftmp, *btmp, *wtmp;

	if (gslb)
	{
		gdk_window_raise(gslb->dbox.window->window);
		return;
	}

	gslb = (struct d_global_servers_list_box *) g_malloc0(sizeof(struct d_global_servers_list_box));

	gslb->Row = -1;
	gslb->dspb = dspb;
	gslb->fsb = fsb;

	gslb->dbox.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_position((GtkWindow *) gslb->dbox.window, GTK_WIN_POS_CENTER);
	gtk_signal_connect((GtkObject *) gslb->dbox.window, "destroy", (GtkSignalFunc) gslb_destroy, (gpointer) gslb);
	gtk_signal_connect((GtkObject *) gslb->dbox.window, "key_press_event", (GtkSignalFunc) dialog_key_press_event, (gpointer) gslb);
	gtk_widget_set_usize(gslb->dbox.window, 610, 300);	/* FIXME */
	gtk_window_set_title((GtkWindow *) gslb->dbox.window, "Predefined list of IRC servers");

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add((GtkContainer *) gslb->dbox.window, vbox);

	ftmp = gtk_frame_new(NULL);
	gtk_container_border_width((GtkContainer *) ftmp, 2);
	gtk_box_pack_start((GtkBox *) vbox, ftmp, TRUE, TRUE, 0);

	btmp = gtk_hbox_new(FALSE, 5);
	gtk_container_border_width((GtkContainer *) btmp, 6);
   gtk_container_add((GtkContainer *) ftmp, btmp);

	gslb->clist = gtk_clist_new_with_titles(4, servers_clist_titles);
	GTK_WIDGET_UNSET_FLAGS(gslb->clist, GTK_CAN_FOCUS);
	gtk_signal_connect((GtkObject *) gslb->clist, "select_row", (GtkSignalFunc) gslb_select_row, (gpointer) gslb);
	gtk_signal_connect((GtkObject *) gslb->clist, "unselect_row", (GtkSignalFunc) gslb_unselect_row, (gpointer) gslb);
	gtk_signal_connect((GtkObject *) gslb->clist, "button_press_event", (GtkSignalFunc) gslb_button_press_event, (gpointer) gslb);
	gtk_clist_column_titles_passive((GtkCList *) gslb->clist);
	gtk_clist_set_column_width((GtkCList *) gslb->clist, 0, 160);
	gtk_clist_set_column_width((GtkCList *) gslb->clist, 1, 130);
	gtk_clist_set_column_width((GtkCList *) gslb->clist, 2, 100);
	gtk_clist_set_column_width((GtkCList *) gslb->clist, 3, 100);

	gtk_box_pack_start((GtkBox *) btmp, gslb->clist, TRUE, TRUE, 0);

	gslb_fill_list(gslb, NULL);

	btmp = gtk_hbox_new(TRUE, 0);
	gtk_container_border_width((GtkContainer *) btmp, 6);
	gtk_box_pack_end((GtkBox *) vbox, btmp, FALSE, FALSE, 0);

	if (dspb) gslb->choose_button = gtk_button_new_with_label(" Choose this server ");
	else gslb->choose_button = gtk_button_new_with_label(" Add this server ");
	GTK_WIDGET_UNSET_FLAGS(gslb->choose_button, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, gslb->choose_button, TRUE, TRUE, 10);
	gtk_signal_connect((GtkObject *) gslb->choose_button, "clicked", (GtkSignalFunc) gslb_add_or_choose, (gpointer) gslb);
	gtk_widget_set_sensitive(gslb->choose_button, FALSE);

	wtmp = gtk_button_new_with_label(" Close this window ");
	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, wtmp, TRUE, TRUE, 10);
	gtk_signal_connect_object((GtkObject *) wtmp, "clicked", (GtkSignalFunc) gtk_widget_destroy, (gpointer) gslb->dbox.window);

	gtk_widget_show_all(gslb->dbox.window);

	if (dspb) dspb->gslb = gslb;
	else if (fsb) fsb->gslb = gslb;
}

/* ----- XXX -------------------------------------------------------------------------- */

//	void dqsb_add_or_choose(GtkWidget *w, gpointer data)
//	{
//		struct d_global_servers_list_box *dqsb = (struct d_global_servers_list_box *) data;
//		struct Global_Server *gs;
//	
//		g_return_if_fail(dqsb->Row != -1);
//	
//		gs = (struct Global_Server *) gtk_clist_get_row_data((GtkCList *) dqsb->clist, dqsb->Row);
//	
//		if (dqsb->dspb)
//		{
//			gtk_entry_set_text((GtkEntry *) ((GtkCombo *) dqsb->dspb->combo_server)->entry, gs->name);
//			gtk_entry_set_text((GtkEntry *) ((GtkCombo *) dqsb->dspb->combo_network)->entry, gs->network);
//			gtk_entry_set_text((GtkEntry *) dqsb->dspb->ports, gs->ports);
//			gtk_toggle_button_set_state((GtkToggleButton *) &((GtkCheckButton *) dqsb->dspb->invisible)->toggle_button, TRUE);
//			gtk_toggle_button_set_state((GtkToggleButton *) &((GtkCheckButton *) dqsb->dspb->wallops)->toggle_button, FALSE);
//			gtk_toggle_button_set_state((GtkToggleButton *) &((GtkCheckButton *) dqsb->dspb->motd)->toggle_button, FALSE);
//			dqsb->dspb->location_hidden = gs->location;
//		}
//		else if (dqsb->fsb)
//		{
//			favorite_server_add_or_update(FALSE, NULL, gs->name, gs->ports, gs->network, gs->location, NULL, NULL, NULL, NULL, FSERVER_INVISIBLE);
//		}
//	
//		gtk_widget_destroy(dqsb->dbox.window);
//	}

//	void dqsb_unselect_row(GtkWidget *w, gint row, gint column, GdkEventButton *v, gpointer data)
//	{
//		struct d_global_servers_list_box *dqsb = (struct d_global_servers_list_box *) data;
//		dqsb->Row = -1;
//		gtk_widget_set_sensitive(dqsb->choose_button, FALSE);
//	}

//	void dqsb_select_row(GtkWidget *w, gint row, gint column, GdkEventButton *v, gpointer data)
//	{
//		struct d_global_servers_list_box *dqsb = (struct d_global_servers_list_box *) data;
//		dqsb->Row = row;
//		gtk_widget_set_sensitive(dqsb->choose_button, TRUE);
//	}

//	gint dqsb_button_press_event(GtkWidget *w, GdkEventButton *e, gpointer data)
//	{
//		struct d_global_servers_list_box *dqsb = (struct d_global_servers_list_box *) data;
//		gint col, row;
//	
//		if (e->button != 1 || (e->type != GDK_2BUTTON_PRESS && e->type != GDK_3BUTTON_PRESS))
//			return FALSE;
//	
//		if (dqsb->Row == -1)
//		{
//			if (!gtk_clist_get_selection_info((GtkCList *) dqsb->clist, e->x, e->y, &row, &col)) return FALSE;
//			dqsb->Row = row;
//		}
//	
//		dqsb_add_or_choose(w, data);
//	
//		return TRUE;
//	}

//	void dqsb_fill_list(struct d_global_servers_list_box *dqsb, gchar *network)
//	{
//		GList *l;
//		gchar *values[4];
//		struct Global_Server *gs;
//		gint row;
//	
//		gtk_clist_clear((GtkCList *) dqsb->clist);
//	
//		l = Global_Servers_List;
//	
//		while (l)
//		{
//			gs = (struct Global_Server *) l->data;
//			values[0] = gs->name;
//			values[1] = gs->ports;
//			values[2] = gs->network;
//			values[3] = gs->location;
//			row = gtk_clist_append((GtkCList *) dqsb->clist, values);
//			gtk_clist_set_row_data((GtkCList *) dqsb->clist, row, (gpointer) gs);
//			l = l->next;
//		}
//	}

struct d_quit_servers_box
{
	struct dbox dbox;
	GtkWidget *entry;
	GList *servers_list;
	Virtual_Window *vw;
	GUI_Window *rw;
	gboolean quit_olirc;
	gchar *reason;
};

struct d_quit_servers_box *dqsb = NULL;

void dqsb_destroy(GtkWidget *w, gpointer data)
{
	g_list_free(dqsb->servers_list);
	g_free_and_NULL(dqsb);
}

void dqsb_do_quit()
{
	GList *l;
	Server *s;

	g_free_and_set(GPrefs.Quit_Reason, g_strdup(dqsb->reason));

	l = dqsb->servers_list;
	while (l)
	{
		s = (Server *) l->data; l = l->next;
		if (s->State != SERVER_IDLE) Server_Quit(s, dqsb->reason);
	}

	if (dqsb->quit_olirc)
	{
		Olirc->Flags |= FLAG_QUITING;
		Prefs_Update();
	}

	if (dqsb->rw) while (dqsb->rw->vw_list) VW_Close((Virtual_Window *) dqsb->rw->vw_list->data);
	else if (dqsb->vw) VW_Close(dqsb->vw);

	if (dqsb->quit_olirc) olirc_real_quit();
}

void dqsb_quit_button(GtkWidget *w, gpointer data)
{
	dqsb->reason = gtk_entry_get_text((GtkEntry *) dqsb->entry);
	dqsb_do_quit();
	gtk_widget_destroy(dqsb->dbox.window);
}

void dqsb_return_func(gpointer data)
{
	dqsb->reason = gtk_entry_get_text((GtkEntry *) dqsb->entry);
	dqsb_do_quit();
	gtk_widget_destroy(dqsb->dbox.window);
}

void dialog_quit_servers(GList *servers_list, gchar *reason, Virtual_Window *vw, GUI_Window *rw, gboolean quit_olirc)
{
	GtkWidget *vbox, *ftmp, *btmp, *wtmp;
	GList *l;

	if (dqsb)
	{
		gdk_window_raise(dqsb->dbox.window->window);
		return;
	}

	g_return_if_fail(servers_list);

	dqsb = (struct d_quit_servers_box *) g_malloc0(sizeof(struct d_quit_servers_box));

	dqsb->quit_olirc = quit_olirc;
	dqsb->rw = rw;
	dqsb->vw = vw;
	dqsb->reason = reason;
	dqsb->servers_list = servers_list;

	dqsb->dbox.return_func = dqsb_return_func;
	dqsb->dbox.stop_focus = TRUE;

	dqsb->dbox.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_position((GtkWindow *) dqsb->dbox.window, GTK_WIN_POS_CENTER);
	gtk_signal_connect((GtkObject *) dqsb->dbox.window, "destroy", (GtkSignalFunc) dqsb_destroy, NULL);
	gtk_signal_connect((GtkObject *) dqsb->dbox.window, "key_press_event", (GtkSignalFunc) dialog_key_press_event, (gpointer) dqsb);

	if (servers_list->next) strcpy(sd_tmp, "Quitting servers");
	else sprintf(sd_tmp, "Quitting server %s", ((Server *) servers_list->data)->fs->Name);

	gtk_window_set_title((GtkWindow *) dqsb->dbox.window, sd_tmp);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add((GtkContainer *) dqsb->dbox.window, vbox);

	ftmp = gtk_frame_new(NULL);
	gtk_container_border_width((GtkContainer *) ftmp, 2);
	gtk_box_pack_start((GtkBox *) vbox, ftmp, TRUE, TRUE, 0);

	btmp = gtk_vbox_new(FALSE, 5);
	gtk_container_border_width((GtkContainer *) btmp, 6);
   gtk_container_add((GtkContainer *) ftmp, btmp);

	if (servers_list->next)
	{
		wtmp = gtk_label_new("You are about to quit servers :");
		gtk_box_pack_start((GtkBox *) btmp, wtmp, TRUE, TRUE, 0);

		l = servers_list;
		while (l)
		{
			sprintf(sd_tmp, ((Server *) l->data)->fs->Name);
			wtmp = gtk_label_new(sd_tmp);
			gtk_box_pack_start((GtkBox *) btmp, wtmp, TRUE, TRUE, 0);
			l = l->next;
		}
	}

	wtmp = gtk_label_new("Please enter the quit reason :");
	gtk_box_pack_start((GtkBox *) btmp, wtmp, TRUE, TRUE, 10);

	dqsb->entry = gtk_entry_new_with_max_length(256);
	gtk_widget_set_usize(dqsb->entry, 500, 0);
	gtk_box_pack_start((GtkBox *) btmp, dqsb->entry, TRUE, TRUE, 10);

	if (!reason) sprintf(sd_tmp, GPrefs.Quit_Reason);
	else strcpy(sd_tmp, reason);
	gtk_entry_set_text((GtkEntry *) dqsb->entry, sd_tmp);
	gtk_entry_select_region((GtkEntry *) dqsb->entry, 0, strlen(sd_tmp));

	gtk_widget_grab_focus(dqsb->entry);

	btmp = gtk_hbox_new(TRUE, 0);
	gtk_container_border_width((GtkContainer *) btmp, 6);
	gtk_box_pack_end((GtkBox *) vbox, btmp, FALSE, FALSE, 0);
	
	wtmp = gtk_button_new_with_label(" Quit ");
	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, wtmp, TRUE, TRUE, 10);
	gtk_signal_connect((GtkObject *) wtmp, "clicked", (GtkSignalFunc) dqsb_quit_button, NULL);

	wtmp = gtk_button_new_with_label(" Cancel ");
	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, wtmp, TRUE, TRUE, 10);
	gtk_signal_connect_object((GtkObject *) wtmp, "clicked", (GtkSignalFunc) gtk_widget_destroy, (gpointer) dqsb->dbox.window);

	gtk_widget_show_all(dqsb->dbox.window);
	gtk_grab_add(dqsb->dbox.window);
}

/* vi: set ts=3: */

