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

/* Management of IRC Servers : GUI part */

#include "olirc.h"
#include "servers.h"
#include "channels.h"
#include "queries.h"
#include "dialogs.h"
#include "windows.h"
#include "network.h"
#include "prefs.h"
#include "histories.h"
#include "dcc.h"
#include "misc.h"
#include "ignores.h"
#include "scripting.h"
#include "servers_dialogs.h"

#include <string.h>
#include <unistd.h>
#include <gdk/gdkkeysyms.h>

GList *Servers_List = NULL;
GList *Server_Open_List = NULL;

gchar srv_tmp[2048];

/* ------------------------------------------------------------------------------------ */

void Server_Close(gpointer data)
{
	Server *s = (Server *) data;
	GList *l;
	DCC *d;

	g_return_if_fail(s->vw->pmask.w_type == W_SERVER);

	if (s->State != SERVER_IDLE) Server_Disconnect(s, TRUE);

	/* Close all the queries of this server */

	while (s->Queries) VW_Close(((Query *) s->Queries->data)->vw);

	/* Close all the channels of this server */

	while (s->Channels) VW_Close(((Channel *) s->Channels->data)->vw);

#ifdef USE_DCC
	/* Find if there is any DCC Chat referencing this server */

	l = DCC_List;

	while (l)
	{
		d = (DCC *) l->data;
		if (d->Type == DCC_CHAT && d->server) if (d->server == s) d->server = NULL;
		l = l->next;
	}
#endif

	/* Remove and free all the Ignores of the server - close windows if needed */

	Server_Ignores_Destroy(s);

	/* Remove the server form the servers list, close its VW and free it */

	Servers_List = g_list_remove(Servers_List, s);

	VW_Remove_From_GW(s->vw, FALSE);
	g_free(s);
}

void Server_Text_Input(gpointer data, gchar *text)
{
	Server *s = (Server *) data;

	VW_output(s->vw, T_WARNING, "st", s, "You can't send any text to a server.");
}

void Server_Infos(gpointer data)
{
	Server *s = (Server *) data;
	gchar *msg = "??";

	gtk_label_set((GtkLabel *) s->vw->WFL[0], s->fs->Name);

	switch (s->State)
	{
		case SERVER_IDLE: msg = "Not connected"; break;
		case SERVER_RESOLVING: msg = "Resolving IP address..."; break;
		case SERVER_CONNECTING: msg = "Connecting..."; break;
		case SERVER_CONNECTED: msg = "Connected"; break;
		case SERVER_DISCONNECTING: msg = "Disconnecting..."; break;
	}

	gtk_label_set((GtkLabel *) s->vw->WFL[1], msg);

	if (s->State == SERVER_CONNECTED && s->current_nick)
	{
		sprintf(srv_tmp, "Your nick is %s", s->current_nick);
		gtk_label_set((GtkLabel *) s->vw->WFL[2], srv_tmp);
		gtk_widget_show(s->vw->WF[2]);
	}
	else gtk_widget_hide(s->vw->WF[2]);
}

/* ----- Callbacks -------------------------------------------------------------------- */

struct server_callback
{
	Server *server;
	gint numeric;
	gchar *mask;
	CallbackFunction(cb_func);
	gpointer data;
	time_t date;
	struct Message *msg;
	gint flags;
};

gpointer Server_Callback_Add(Server *s, gint num, gchar *mask, gint Flags, CallbackFunction(cb_func), gpointer data)
{
	struct server_callback *sc;

	g_return_val_if_fail(s, NULL);
	g_return_val_if_fail(cb_func, NULL);

	#ifdef DEBUG_CALLBACKS
	fprintf(stdout, "Server_Callback_Add(%s, %d, %s, %d, %p, %p)\n", s->fs->Name, num, mask, Flags, cb_func, data);
	#endif

	sc = (struct server_callback *) Server_Callback_Check(s, num, mask, cb_func);

	if (sc)
	{
		if (!(sc->flags & CBF_QUIET))
		{
			gint t = CALLBACK_LIFETIME - (time(NULL) - sc->date);
			VW_output(s->vw_active, T_WARNING, "sF--", s, "Waiting %d second%s for previous request...", t, (t>1)? "s" : "");
		}
		return NULL;
	}

	sc = (struct server_callback *) g_malloc0(sizeof(struct server_callback));

	sc->server = s;
	sc->numeric = num;
	sc->mask = g_strdup(mask);
	sc->cb_func = cb_func;
	sc->data = data;
	sc->date = time(NULL);
	sc->flags = Flags;

	s->Callbacks = g_list_append(s->Callbacks, (gpointer) sc);

	return sc;
};

gpointer Server_Callback_Check(Server *s, gint num, gchar *mask, CallbackFunction(cb_func))
{
	GList *l;
	struct server_callback *sc;

	g_return_val_if_fail(s, NULL);

	#ifdef DEBUG_CALLBACKS
	fprintf(stdout, "Server_Callback_Check(%s, %d, %s, %p)\n", s->fs->Name, num, mask, cb_func);
	#endif

	l = s->Callbacks;

	while (l)
	{
		sc = (struct server_callback *) l->data; 
		l = l->next;

		if ((time(NULL) - sc->date) >= CALLBACK_LIFETIME)
		{
			Server_Callback_Remove((gpointer) sc);
		}
		else if (sc->numeric == num)
		{
			if ( (sc->mask && mask && !irc_cmp(sc->mask, mask)) || (!sc->mask && !mask) )
			{
				if (!cb_func) return sc;
				if (sc->cb_func == cb_func) return sc;
			}
		}
	}

	return NULL;
}

void Server_Callback_Remove(gpointer sc_pointer)
{
	struct server_callback *sc = (struct server_callback *) sc_pointer;

	g_return_if_fail(sc);

	#ifdef DEBUG_CALLBACKS
	fprintf(stdout, "Server_Callback_Remove(%p)\n", sc_pointer);
	#endif

	sc->server->Callbacks = g_list_remove(sc->server->Callbacks, sc);

	g_free(sc->mask);
	g_free(sc);
}

gboolean Server_Callback_Call(gpointer sc_pointer, struct Message *m)
{
	gboolean keep_processing;
	struct server_callback *sc = (struct server_callback *) sc_pointer;

	g_return_val_if_fail(sc, FALSE);
	g_return_val_if_fail(sc->cb_func, FALSE);

	#ifdef DEBUG_CALLBACKS
	fprintf(stdout, "Server_Callback_Call(%p, %p)\n", sc_pointer, m);
	#endif

	keep_processing = sc->flags & CBF_KEEP_PROCESSING;

	if (!sc->cb_func(m, sc->data)) Server_Callback_Remove(sc);

	return keep_processing;
}

/* ----- Operations on servers -------------------------------------------------------- */

void Server_Disconnect(Server *s, gboolean forced)
{
	GList *l;
	Channel *c;
	Query *q;
	gint old_state;

	g_return_if_fail(s);

	old_state = s->State;

	if (old_state == SERVER_IDLE)
	{
		g_warning("Server_Disconnect(%s): Attempt to disconnect an idle server !?",s->fs->Name);
		return;
	}

	if (s->fd && s->fd!=-1)
	{
		if (s->gdk_tag != -1) gdk_input_remove(s->gdk_tag);
		close(s->fd);
		s->fd = 0;
		s->gdk_tag = -1;
	}

	if (s->nick_box) gtk_widget_destroy(((struct dbox *) s->nick_box)->window);

	s->State = SERVER_IDLE;
	s->current_nick = NULL;
	VW_Status(s->vw);

	if (old_state == SERVER_DISCONNECTING) VW_output(s->vw, T_ERROR, "st", s, "Connection closed");
	else VW_output(s->vw, T_ERROR, "st", s, "Connection aborted");

	if (old_state == SERVER_CONNECTED || old_state == SERVER_DISCONNECTING)
	{
		/* If the server was connected, show the deconnection in all windows */

		l = s->Channels;
		while (l)
		{
			c = (Channel *) l->data;
			c->State = CHANNEL_NOT_JOINED;
			Channel_Members_ClearList(c);
			VW_output(c->vw, T_ERROR, "#st", s, "Disconnected from server");
			VW_Status(c->vw);
			l = l->next;
		}

		l = s->Queries;
		while (l)
		{
			q = (Query *) l->data;
			VW_output(q->vw, T_ERROR, "#st", s, "Disconnected from server");
			VW_Status(q->vw);
			l = l->next;
		}
	}

	while (s->Callbacks) Server_Callback_Remove(s->Callbacks->data);

	Event_Raise(EV_SERVER_DISCONNECTED, s->vw, s, NULL, NULL, NULL, NULL);

	g_free_and_NULL(s->Input_Buffer);
	g_free_and_NULL(s->Output_Buffer);
}

void Server_Quit(Server *s, gchar *reason)
{
	if (s->State!=SERVER_CONNECTED && s->State != SERVER_CONNECTING)
	{
		g_warning("Server_Quit(%s): Attempt to quit a non-connected server !?",s->fs->Name);
		return;
	}

	if (Event_Raise(EV_SERVER_QUIT, s->vw, s, NULL, NULL, NULL, reason)) return;

	s->State = SERVER_DISCONNECTING;
	VW_Status(s->vw);
	VW_output(s->vw, T_WARNING, "st", s, "Disconnecting...");
	sprintf(srv_tmp,"\nQUIT :%s", reason);
	Server_Output(s, srv_tmp, FALSE);
}

guint Server_Get_Port(Server *s)
{
	// gchar *r;

	g_return_val_if_fail(s, 0);
	g_return_val_if_fail(s->fs, 0);
	g_return_val_if_fail(s->fs->Ports, 0);

/*	r = s->fs->Ports;

	if (*r <

*/

	return atoi(s->fs->Ports);
}

Server *Server_find_by_id(guint32 id)
{
	GList *l = Servers_List;
	while (l)
	{
		if (((Server *) l->data)->object_id == id) return (Server *) l->data;
		l = l->next;
	}
	return NULL;
}

Server *Server_New(Favorite_Server *fs, gboolean activate, GUI_Window *rw)
{
	Server *s;
	static guint32 srv_id = 0;

	if (!(fs->Name) || !(fs->Ports) || !(fs->Nick)) return NULL;
	if (!(*fs->Name) || !(*fs->Ports) || !(*fs->Nick)) return NULL;

	if (!atoi(fs->Ports)) return NULL;

	s = (Server *) g_malloc0(sizeof(Server));

	s->object_id = srv_id++;
	s->State = SERVER_IDLE;
	s->fs = fs;

	s->gdk_tag = -1;

	Servers_List = g_list_append(Servers_List, s);

	s->vw = VW_new();
	s->vw->pmask.w_type = W_SERVER;
	s->vw->pmask.network = s->fs->Network;
	s->vw->pmask.server = s->fs->Name;
	s->vw->Name = s->fs->Name;
	s->vw->Title = s->fs->Name;
	s->vw->Resource = (gpointer) s;
	s->vw->Infos = Server_Infos;
	s->vw->Close = Server_Close;
	s->vw->Input = Server_Text_Input;

	if (!rw && GW_List) rw = (GUI_Window *) GW_List->data;
	VW_Init(s->vw, rw, 3);

	gtk_widget_show_all(s->vw->Page_Box);

	if (activate) VW_Activate(s->vw);

	return s;
}

void Server_Init(Server *s, gchar *nick) 
{
   /* Initialize some server parameters */

   Channel *c;
   GList *l;
	gboolean send_join = FALSE;

   /* Set actual nick */

   s->current_nick = g_strdup(nick);
   VW_Status(s->vw);

	Event_Raise(EV_SERVER_CONNECTED, s->vw, s, NULL, NULL, NULL, NULL);

   /* Set modes */

   sprintf(srv_tmp,"MODE %s %ci %cw", s->current_nick,
		(s->fs->Flags & FSERVER_INVISIBLE)? '+' : '-',
		(s->fs->Flags & FSERVER_WALLOPS)? '+' : '-');

	Server_Output(s, srv_tmp, TRUE);

   /* Join all unjoined channels */

	strcpy(srv_tmp, "JOIN ");

   l = s->Channels;
   while (l)
   {
      c = (Channel *) l->data;
      if (c->State != CHANNEL_JOINED)
		{
			strcat(srv_tmp, c->Name);
			strcat(srv_tmp, ",");
			send_join = TRUE;
		}
      l = l->next;
   }

	if (send_join)
	{
		srv_tmp[strlen(srv_tmp)-1] = 0;
		Server_Output(s, srv_tmp, TRUE);
	}
}

void Servers_Connect_All()
{
	GList *l = Servers_List;
	Server *s;

	while (l)
	{
		s = (Server *) l->data;
		if (s->State == SERVER_IDLE) Server_Connect(s);
		l = l->next;
	}
}

void Servers_Reopen()
{
	if (!Server_Open_List) return;

	while (Server_Open_List)
	{
		Favorite_Server *fs = favorite_server_find((gchar *) Server_Open_List->data);
		if (fs)
		{
			Server *s;
			GList *l = Channel_Open_List;

			s = Server_New(fs, TRUE, NULL);
			if (!s)
			{
				sprintf(srv_tmp, "Can't reopen server '%s' !\n(Error in Preferences file)", fs->Name);
				Message_Box("Error", srv_tmp);
				break;
			}
			VW_output(s->vw, T_WARNING, "#st", s, "\nTo connect, right-click on the server's tab, then choose 'Server->Connect'.\n");

			while (l)
			{
				if (!strncmp(Server_Open_List->data, l->data, strlen(Server_Open_List->data)))
				{
					GList *k = l->next;
					Channel *c = Channel_New(s, l->data+strlen(Server_Open_List->data));
					VW_output(c->vw, T_WARNING, "#st", s, "Channel will be joined on server connection.");
					g_free(l->data);
					Channel_Open_List = g_list_remove_link(Channel_Open_List, l);
					l = k;
				}
				else l = l->next;
			}

			s->vw_active = s->vw;
		}
		else fprintf(stderr, "Can't reopen server %s: not found in Favorite Servers List\n", (gchar *) Server_Open_List->data);

		g_free(Server_Open_List->data);
		Server_Open_List = g_list_remove(Server_Open_List, Server_Open_List->data);
	}
}

/* ----- Management of Raw logs windows ----------------------------------------- */

void Raw_Window_delete_event(GtkWidget *w, GdkEvent *v, gpointer data)
{
	Server *s = (Server *) data;
	#ifdef DEBUG_SERVERS
	fprintf(stdout, "Server_Raw_Window_delete_event(%p, %p, %p [%s])\n", w, v, data, s->fs->Name);
	#endif
	gtk_widget_destroy(s->Raw_Window);
	s->Raw_Window = NULL;
}

void Server_Raw_Window_Output(Server *s, char *text, gboolean sending)
{
	gchar **tt, **ts;

	guchar *seek, *start, v;

	#ifdef DEBUG_SERVERS
	fprintf(stdout, "Server_Raw_Window_Output(%p [%s], %s, %d)\n", s, s->fs->Name, text, sending);
	#endif

	g_return_if_fail(s->Raw_Window);

	tt = g_strsplit(text, "\n", -1);

	for (ts = tt; *ts; ts++)
	{
		text = *ts;

		if (sending) sprintf(srv_tmp, "< "); else sprintf(srv_tmp, "> ");

		seek = start = (guchar *) text;

		while (*seek)
		{
			if (*seek < ' ')
			{
				v = *seek; *seek = 0;
				strcat(srv_tmp, (const char *) start);
				gtk_text_insert(GTK_TEXT(s->Raw_Text), NULL, &Olirc->Colors[1], &Olirc->Colors[0], srv_tmp, -1);
				sprintf(srv_tmp, "%.2X", (guint) v);
				gtk_text_insert(GTK_TEXT(s->Raw_Text), NULL, &Olirc->Colors[5], &Olirc->Colors[15], srv_tmp, -1);
				start = ++seek; *srv_tmp = 0;
			}
			else seek++;
		}

		if (*start) strcat(srv_tmp, (const char *) start);
		gtk_text_insert(GTK_TEXT(s->Raw_Text), NULL, &Olirc->Colors[1], &Olirc->Colors[0], srv_tmp, -1);
		gtk_text_insert(GTK_TEXT(s->Raw_Text), NULL, &Olirc->Colors[5], &Olirc->Colors[15], "0A\n", 3);

	}
	g_strfreev(tt);
}

void Server_Display_Raw_Window(Server *s)
{
	GtkWidget *box, *scroll;

	#ifdef DEBUG_SERVERS
	fprintf(stdout, "Server_Display_Raw_Window(%p [%s])\n", s, s->fs->Name);
	#endif

	if (s->Raw_Window)
	{
		gdk_window_raise(s->Raw_Window->window);
		return;
	}

	s->Raw_Window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect(GTK_OBJECT(s->Raw_Window), "delete_event", GTK_SIGNAL_FUNC(Raw_Window_delete_event), (gpointer) s);
	gtk_widget_set_usize(s->Raw_Window, 600, 150);
	gtk_window_set_policy(GTK_WINDOW(s->Raw_Window), TRUE, TRUE, FALSE);
	gtk_container_border_width(GTK_CONTAINER(s->Raw_Window), 2);
	sprintf(srv_tmp, "Raw log of server %s", s->fs->Name);
	gtk_window_set_title(GTK_WINDOW(s->Raw_Window), srv_tmp);

   box = gtk_hbox_new(FALSE,0);
   gtk_container_add(GTK_CONTAINER(s->Raw_Window), box);

   s->Raw_Text = gtk_text_new(NULL, NULL);
   GTK_WIDGET_UNSET_FLAGS(s->Raw_Text, GTK_CAN_FOCUS);
   gtk_box_pack_start(GTK_BOX(box), s->Raw_Text, TRUE, TRUE, 0);

   scroll = gtk_vscrollbar_new(GTK_TEXT(s->Raw_Text)->vadj);
   GTK_WIDGET_UNSET_FLAGS(scroll, GTK_CAN_FOCUS);
   gtk_box_pack_start(GTK_BOX(box), scroll, FALSE, FALSE, 0);

	gtk_widget_show_all(s->Raw_Window);
}

/* vi: set ts=3: */

