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
#include "windows.h"
#include "channels.h"
#include "misc.h"
#include "network.h"
#include "prefs.h"
#include "signal.h"
#include "histories.h"
#include "menus.h"
#include "queries.h"
#include "scripting.h"
#include "servers.h"

#include <string.h>

gchar channel_tmp[1024];
gchar channel_tmp2[1024];

GList *Channel_Open_List = NULL;

void Channel_Change_Topic(GtkWidget *, gpointer);
gint Channel_Members_button_press_event(GtkWidget *, GdkEventButton *, gpointer);

/* ------------------------------------------------------------------------------------ */

void Channel_Input(gpointer data, gchar *text)
{
	Channel *c = (Channel *) data;

	g_return_if_fail(text);

	if (c->State != CHANNEL_JOINED)
	{
		VW_output(c->vw, T_WARNING, "sF-", c->server, "%s", "Channel not joined.");
		return;
	}

	g_return_if_fail(c->server->current_nick);

	if (*text) sprintf(channel_tmp,"PRIVMSG %s :%s", c->Name, text);
	else sprintf(channel_tmp,"PRIVMSG %s : ", c->Name);
	if (!Server_Output(c->server, channel_tmp, TRUE)) return;

	VW_output(c->vw, T_OWN_CHAN_MSG, "snudt", c->server, c->server->current_nick, "userhost", c->Name, text);
}

void Channel_Close(gpointer data)
{
	Channel *c = (Channel *) data;

	g_return_if_fail(c);

	if (c->vw->rw->vw_active == c->vw)
		if (!GW_Activate_Last_VW(c->vw->rw)) VW_Activate(c->server->vw);

	if (c->State==CHANNEL_JOINED && c->server->State==SERVER_CONNECTED)
	{
		sprintf(channel_tmp,"PART %s",c->Name);
		Server_Output(c->server,channel_tmp, FALSE);
	}

	Channel_Members_ClearList(c); /* Free all members of the channel */

	c->server->Channels = g_list_remove(c->server->Channels, c);
	VW_Remove_From_GW(c->vw, FALSE);

	g_free(c->Mode);
	g_free(c->Topic);
	g_free(c);
}

void Channel_Infos(gpointer data)
{
	Channel *c = (Channel *) data;
	gint i;

	g_return_if_fail(c);

	gtk_label_set((GtkLabel *) c->vw->WFL[0], c->server->fs->Name);

	if (c->server->State != SERVER_CONNECTED)
	{
		gtk_label_set((GtkLabel *) c->vw->WFL[1], "Not connected");
		for (i=2; i<7; i++) gtk_widget_hide(c->vw->WF[i]);
	}
	else
	{
		gchar *r = NULL;
		switch (c->State)
		{
			case CHANNEL_TOOMANYCHANS: r = "You have joined too many channels"; break;
			case CHANNEL_BAD_KEY: r = "Channel is key-protected"; break;
			case CHANNEL_UNAVAILABLE: r = "Channel is temporarily unavailable"; break;
			case CHANNEL_FULL: r = "Channel is full"; break;
			case CHANNEL_INVITE_ONLY: r = "Channel is invite only"; break;
			case CHANNEL_BANNED: r = "You are banned"; break;
			case CHANNEL_KICKED: r = "You have been kicked"; break;
			case CHANNEL_JOINING: r = "Joining channel..."; break;
			case CHANNEL_REJOINING: r = "Attempting to rejoin..."; break;
			case CHANNEL_NOT_JOINED: r = "Channel not joined"; break;
			case CHANNEL_JOINED:
				r = c->Name;
				if (c->Mode)
				{
					sprintf(channel_tmp, "+%s", c->Mode);
					if (c->Key)
					{
						sprintf(channel_tmp2, " +k %s", c->Key);
						strcat(channel_tmp, channel_tmp2);
					}
					if (c->Limit)
					{
						sprintf(channel_tmp2, " +l %d", c->Limit);
						strcat(channel_tmp, channel_tmp2);
					}
					gtk_label_set((GtkLabel *) c->vw->WFL[2], channel_tmp);
					gtk_widget_show(c->vw->WF[2]);
				}
				else gtk_widget_hide(c->vw->WF[2]);
				sprintf(channel_tmp,"%d member%s", c->Users, (c->Users>1)? "s":"");
				gtk_label_set((GtkLabel *) c->vw->WFL[3], channel_tmp);
				if (c->Ops) sprintf(channel_tmp,"%d op%s", c->Ops, (c->Ops>1)? "s":"");
				else strcpy(channel_tmp, "no ops");
				gtk_label_set((GtkLabel *) c->vw->WFL[4], channel_tmp);
				if (c->Voiced) sprintf(channel_tmp,"%d voiced", c->Voiced);
				else strcpy(channel_tmp, "no voiced");
				gtk_label_set((GtkLabel *) c->vw->WFL[5], channel_tmp);
				sprintf(channel_tmp,"%d other%s", c->Users-(c->Ops+c->Voiced), ((c->Users-(c->Ops+c->Voiced))>1)? "s":"");
				gtk_label_set((GtkLabel *) c->vw->WFL[6], channel_tmp);
				break;
		}

		if (r) gtk_label_set((GtkLabel *) c->vw->WFL[1], r);

		if (c->State != CHANNEL_JOINED) for (i=2; i<7; i++) gtk_widget_hide(c->vw->WF[i]);
		else for (i=3; i<7; i++) gtk_widget_show(c->vw->WF[i]);
	}

	if (!(c->Flags & CHANNEL_EDITING_TOPIC))
		gtk_entry_set_text(GTK_ENTRY(c->vw->Topic), (c->Topic)? c->Topic : "");

	gtk_widget_set_sensitive(c->vw->Topic, c->State == CHANNEL_JOINED);
}

/* ------------------------------------------------------------------------------------ */

void Channel_Join(Channel *c, gchar *key)
{
	g_return_if_fail(c);

	if (c->State!=CHANNEL_JOINED && c->server->State==SERVER_CONNECTED)
	{
		if (key) sprintf(channel_tmp,"JOIN %s %s", c->Name, key);
		else sprintf(channel_tmp,"JOIN %s", c->Name);
		Server_Output(c->server,channel_tmp, FALSE);
		c->State = CHANNEL_JOINING;
		VW_Status(c->vw);
	}
	else if (c->server->State!=SERVER_CONNECTED)
	{
		VW_output(c->vw, T_WARNING, "sF-", c->server, "Can't join %s now: Not connected to the server.", c->Name);
		VW_output(c->vw, T_WARNING, "sF-", c->server, "Please establish the connection with %s first.", c->server->fs->Name);
	}
}

void Channel_Part(Channel *c, gchar *reason)
{
	g_return_if_fail(c);

	if (c->server->State==SERVER_CONNECTED)
	{
		if (reason) sprintf(channel_tmp,"PART %s :%s",c->Name, reason);
		else sprintf(channel_tmp,"PART %s",c->Name);
		Server_Output(c->server,channel_tmp, FALSE);
	}
}

gint Channel_Members_button_press_event(GtkWidget *w, GdkEventButton *e, gpointer data)
{
	Channel *c = (Channel *) data;
	Member *m;
	gint row, col;

	g_return_val_if_fail(c, FALSE);

	if (e->window != GTK_CLIST(w)->clist_window) return FALSE;

	if (!gtk_clist_get_selection_info(GTK_CLIST(c->vw->Members), e->x, e->y, &row, &col)) return FALSE;

	m = (Member *) gtk_clist_get_row_data(GTK_CLIST(c->vw->Members), row);

	if (e->button == 1 && (e->type==GDK_2BUTTON_PRESS || e->type==GDK_3BUTTON_PRESS))
	{
		Query_New(c->server, m->nick);
	}
	else if (e->button == 3)
	{
		Popupmenu_Member_Display(c, m, e);
		gtk_signal_emit_stop_by_name((GtkObject *) w, "button_press_event");
	}

	return FALSE;
}

Channel *Channel_find_by_id(guint32 id)
{
	GList *l = Servers_List;
	GList *m;
	while (l)
	{
		m = ((Server *) l->data)->Channels;
		while (m)
		{
			if (((Channel *) m->data)->object_id == id) return (Channel *) m->data;
			m = m->next;
		}
		l = l->next;
	}
	return NULL;
}

Channel *Channel_New(Server *s, gchar *Name)
{
	static guint32 ch_id = 0;

	Channel *c;

	g_return_val_if_fail(s, NULL);

	c = Channel_Find(s, Name);

	if (!c)
	{
		GtkWidget *vbox, *wtmp;

		c = (Channel *) g_malloc0(sizeof(Channel));
		c->object_id = ch_id++;
		c->Name = g_strdup(Name);
		c->server = s;
		c->State = CHANNEL_NOT_JOINED;

		c->vw = VW_new();
		c->vw->pmask.w_type = W_CHANNEL;
		c->vw->pmask.server = s->fs->Name;
		c->vw->pmask.network = s->fs->Network;
		c->vw->pmask.channel = c->Name;
		c->vw->Resource = (gpointer) c;
		c->vw->Name = c->Name;
		c->vw->Title = c->Name;
		c->vw->Infos = Channel_Infos;
		c->vw->Close = Channel_Close;
		c->vw->Input = Channel_Input;

		VW_Init(c->vw, s->vw->rw, 7);

		wtmp = gtk_label_new("Topic");
		gtk_box_pack_start((GtkBox *) c->vw->Head_Box, wtmp, FALSE, FALSE, 8);
		c->vw->Topic = gtk_entry_new_with_max_length(256);
		gtk_signal_connect((GtkObject *) c->vw->Topic, "activate", GTK_SIGNAL_FUNC(Channel_Change_Topic), (gpointer) c);
		gtk_signal_connect((GtkObject *) c->vw->Topic, "focus_in_event", GTK_SIGNAL_FUNC(Channel_Topic_Focus_In), (gpointer) c);
		gtk_signal_connect((GtkObject *) c->vw->Topic, "focus_out_event", GTK_SIGNAL_FUNC(Channel_Topic_Focus_Out), (gpointer) c);
		gtk_box_pack_start((GtkBox *) c->vw->Head_Box, c->vw->Topic, TRUE, TRUE, 0);

		vbox = gtk_vbox_new(FALSE, 0);
		gtk_paned_pack2 (GTK_PANED (c->vw->pane), vbox, FALSE, TRUE);
		c->vw->Members = gtk_clist_new(1);
		GTK_WIDGET_UNSET_FLAGS(c->vw->Members, GTK_CAN_FOCUS);

		c->vw->Members_Scrolled = gtk_scrolled_window_new(NULL, NULL);
		gtk_widget_set_usize((GtkWidget *) c->vw->Members_Scrolled, 136, 0);
		gtk_clist_set_column_width((GtkCList *) c->vw->Members, 0, 106);
		gtk_container_add((GtkContainer *) c->vw->Members_Scrolled, c->vw->Members);
		gtk_scrolled_window_set_policy((GtkScrolledWindow *) c->vw->Members_Scrolled, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		GTK_WIDGET_UNSET_FLAGS(((GtkScrolledWindow *) c->vw->Members_Scrolled)->vscrollbar, GTK_CAN_FOCUS);
		GTK_WIDGET_UNSET_FLAGS(((GtkScrolledWindow *) c->vw->Members_Scrolled)->hscrollbar, GTK_CAN_FOCUS);
		gtk_box_pack_start((GtkBox *) vbox, c->vw->Members_Scrolled, TRUE, TRUE, 0);

		gtk_clist_set_column_justification((GtkCList *) c->vw->Members, 0, GTK_JUSTIFY_CENTER);

		c->Prefs = PREF_CHAN_VIEW_JOIN | PREF_CHAN_VIEW_PART | PREF_CHAN_VIEW_QUIT |
			PREF_CHAN_VIEW_NICK | PREF_CHAN_VIEW_MODE | PREF_CHAN_VIEW_KICK |
			PREF_CHAN_VIEW_TOPIC;

		gtk_signal_connect((GtkObject *) c->vw->Members, "button_press_event", (GtkSignalFunc) Channel_Members_button_press_event, (gpointer) c);

		gtk_widget_show_all(c->vw->Page_Box);

		s->Channels = g_list_append(s->Channels, c);

		VW_Activate(c->vw);
	}
	else if (c->vw->rw->vw_active != c->vw) VW_Activate(c->vw);

	return c;
}

Channel *Channel_Find(Server *s, gchar *name)
{
	GList *l;
	Channel *c;

	g_return_val_if_fail(s, NULL);

	l = s->Channels;

	while (l)
	{
		c = (Channel *) l->data;
		if (!irc_cmp(name, c->Name)) return c;
		l = l->next;
	}

	return NULL;
}

void Channel_Topic_Focus_In(GtkWidget *w, GdkEvent *v, gpointer data)
{
	g_return_if_fail(data);
	((Channel *) data)->Flags |= CHANNEL_EDITING_TOPIC;
}

void Channel_Topic_Focus_Out(GtkWidget *w, GdkEvent *v, gpointer data)
{
	Channel *c = (Channel *) data;
	gchar *t;
	g_return_if_fail(c);
	c->Flags &= ~CHANNEL_EDITING_TOPIC;

	t = gtk_entry_get_text((GtkEntry *) c->vw->Topic);
	if ((c->Topic && strcmp(t, c->Topic)) || (!c->Topic && *t))
		gtk_entry_set_text((GtkEntry *) c->vw->Topic, (c->Topic)? c->Topic : "");
}

void Channel_Change_Topic(GtkWidget *w, gpointer data)
{
	Channel *c = (Channel *) data;
	g_return_if_fail(c);
	sprintf(channel_tmp, "TOPIC %s :%s", c->Name, gtk_entry_get_text((GtkEntry *) w));
	Server_Output(c->server,channel_tmp, TRUE);
	VW_output(c->vw, T_TOPIC, "sF-", c->server, "%s", "Changing topic...");
	gtk_widget_grab_focus(c->vw->entry);
}

/* ----- Management of channels members lists ----------------------------------------- */

gint Channel_Member_C_Compare(const gpointer a, const gpointer b)
{
	return irc_cmp(((Member *) a)->nick, ((Member *) b)->nick);
	/* <PREFS> Sorting of completion members lists */
}

gint Channel_Member_Compare(const gpointer a, const gpointer b)
{
	if (HAS_OP((Member *) a) && HAS_OP((Member *) b))
		return irc_cmp(((Member *) a)->nick, ((Member *) b)->nick);
	else if (HAS_OP((Member *) a)) return 0;
	else if (HAS_OP((Member *) b)) return 1;
	else if (HAS_VOICE((Member *) a) && HAS_VOICE((Member *) b))
		return irc_cmp(((Member *) a)->nick, ((Member *) b)->nick);
	else if (HAS_VOICE((Member *) a)) return 0;
	else if (HAS_VOICE((Member *) b)) return 1;
	else return irc_cmp(((Member *) a)->nick, ((Member *) b)->nick);

	/* <PREFS> Sorting of channel members lists */
}

void Channels_Member_Userhost_Store(Server *s, gchar *user)
{
	register GList *l, *m;
	register gchar *nick;

	g_return_if_fail(s);
	g_return_if_fail(user);

	if (!(l = s->Channels)) return;

	nick = nick_part(user);
	
	if (strchr(nick, '.')) return;

	while (l)
	{
		m = ((Channel *) l->data)->Members;

		while (m)
		{
			if (!irc_cmp(((Member *) m->data)->nick, nick))
			{
				if (!((Member *) m->data)->userhost) ((Member *) m->data)->userhost = g_strdup(userhost_part(user));
				break;
			}
			m = m->next;
		}

		l = l->next;
	}
}

Member *Channel_Member_Find(Channel *c, gchar *user)
{
	GList *l;
	gchar *nick;

	g_return_val_if_fail(c, NULL);
	g_return_val_if_fail(user, NULL);

	if (!(l = c->Members)) return NULL;

	nick = nick_part(user);

	while (l)
	{
		if (!irc_cmp(((Member *) l->data)->nick, nick))
		{
			if (!((Member *) l->data)->userhost) ((Member *) l->data)->userhost = g_strdup(userhost_part(user));
			return (Member *) l->data;
		}
		l = l->next;
	}

	return NULL;
}

void Channel_Member_Set(Channel *c, Member *m, gint index, gboolean insert)
{
	gchar *nick = channel_tmp;

	/* <PREFS> If the user wants to display xpm icons instead of a @ or + chars ... */
	
	if (HAS_OP(m)) sprintf(channel_tmp, "@%s", m->nick);
	else if (HAS_VOICE(m)) sprintf(channel_tmp, "+%s", m->nick);
	else sprintf(channel_tmp, "%s", m->nick);

	if (insert)
	{
		gtk_clist_insert((GtkCList *) c->vw->Members, index, &nick);
		gtk_clist_set_row_data((GtkCList *) c->vw->Members, index, (gpointer) m);
	}
	else gtk_clist_set_text((GtkCList *) c->vw->Members, index, 0, nick);
}

void Channel_Member_Update(Channel *c, Member *m, gchar *nick, gint state)
{
	gboolean need_move = FALSE;
	gint index, new_index;

	g_return_if_fail(c);
	g_return_if_fail(m);

	if (nick)
	{
		if (irc_cmp(m->nick, nick)) need_move = TRUE;
		g_free(m->nick); m->nick = g_strdup(nick);
	}

	if ((state & FLAG_OP && !HAS_OP(m)))
	{
		if (HAS_VOICE(m)) c->Voiced--;
		m->State |= FLAG_OP; need_move = TRUE;
		c->Ops++;
	}
	else if (HAS_OP(m) && !(state & FLAG_OP))
	{
		m->State &= ~FLAG_OP; need_move = TRUE;
		c->Ops--;
		if (HAS_VOICE(m)) c->Voiced++;
	}

	if ((state & FLAG_VOICE && !HAS_VOICE(m)))
	{
		m->State |= FLAG_VOICE; need_move = TRUE;
		if (!HAS_OP(m)) c->Voiced++;
	}
	else if (HAS_VOICE(m) && !(state & FLAG_VOICE))
	{
		m->State &= ~FLAG_VOICE; need_move = TRUE;
		if (!HAS_OP(m)) c->Voiced--;
	}

	index = g_list_index(c->Members, m);

	if (need_move)
	{
		c->C_Members = g_list_remove(c->C_Members, (gpointer) m);
		c->C_Members = g_list_insert_sorted(c->C_Members, (gpointer) m, (GCompareFunc) Channel_Member_C_Compare);

		c->Members = g_list_remove(c->Members, (gpointer) m);
		c->Members = g_list_insert_sorted(c->Members, (gpointer) m, (GCompareFunc) Channel_Member_Compare);

		new_index = g_list_index(c->Members, m);

		if (new_index != index)
		{
			gtk_clist_remove((GtkCList *) c->vw->Members, index);
			Channel_Member_Set(c, m, new_index, TRUE);
		}
		else Channel_Member_Set(c, m, index, FALSE);
	}
	else Channel_Member_Set(c, m, index, FALSE);

	VW_Status(c->vw);
}

gboolean Channel_Add_User(Channel *c, gchar *user)
{
	Member *m;
	gint state = 0;

	g_return_val_if_fail(c, FALSE);
	g_return_val_if_fail(user, FALSE);

	if (*user == '@') { user++; state = FLAG_OP; }
	else if (*user == '+') { user++; state = FLAG_VOICE; }

	if ((m = Channel_Member_Find(c, user)))
	{
		Channel_Member_Update(c, m, NULL, m->State | state);
		return FALSE;
	}

	m = (Member *) g_malloc0(sizeof(Member));

	m->nick = g_strdup(nick_part(user));
	m->userhost = g_strdup(userhost_part(user));
	m->State = state;

	c->C_Members = g_list_insert_sorted(c->C_Members, (gpointer) m, (GCompareFunc) Channel_Member_C_Compare);

	c->Members = g_list_insert_sorted(c->Members, (gpointer) m, (GCompareFunc) Channel_Member_Compare);

	Channel_Member_Set(c, m, g_list_index(c->Members, m), TRUE);

	c->Users++;

	if (HAS_OP(m)) c->Ops++;
	else if (HAS_VOICE(m)) c->Voiced++;

	return TRUE;
}

gboolean Channel_Remove_User(Channel *c, gchar *user)
{
	Member *m;

	g_return_val_if_fail(c, FALSE);
	g_return_val_if_fail(user, FALSE);

	if (!(m = Channel_Member_Find(c, user))) return FALSE;

	c->C_Members = g_list_remove(c->C_Members, (gpointer) m);
	c->Members = g_list_remove(c->Members, (gpointer) m);

	gtk_clist_remove((GtkCList *) c->vw->Members, gtk_clist_find_row_from_data((GtkCList *) c->vw->Members, (gpointer) m));

	if (HAS_OP(m)) c->Ops--;
	else if (HAS_VOICE(m)) c->Voiced--;

	c->Users--;

	g_free(m->nick);
	g_free(m->userhost);
	g_free(m);

	return TRUE;
}

void Channel_Members_ClearList(Channel *c)
{
	g_return_if_fail(c);

	while (c->Members)
	{
		c->C_Members = g_list_remove(c->C_Members, c->Members->data);
		g_free(((Member *) c->Members->data)->nick);
		g_free(((Member *) c->Members->data)->userhost);
		g_free(c->Members->data);
		c->Members = g_list_remove(c->Members, c->Members->data);
	}

	gtk_clist_clear((GtkCList *) c->vw->Members);

	c->Users = 0; c->Ops = 0; c->Voiced = 0;
}

void Channel_Members_Reset(Channel *c)
{
	Member *m;
	GList *l;
	gint r;

	g_return_if_fail(c);

	if (c->vw->Member_Index)
	{
		c->vw->Member_Index = NULL;
		g_free(c->vw->Before); g_free(c->vw->Completing); g_free(c->vw->After);
	}

	gtk_clist_clear((GtkCList *) c->vw->Members);

	l = c->Members;

	while (l)
	{
		m = (Member *) l->data;
		r = gtk_clist_append(GTK_CLIST(c->vw->Members), &m->nick);
		gtk_clist_set_row_data(GTK_CLIST(c->vw->Members), r, (gpointer) m);
		l = l->next;
	}
}

/* ----- Management of messages to channels ------------------------------------------- */

void Channel_Msg_Names(Server *s, gchar *channel, gchar *list)
{
	Channel *c = Channel_Find(s, channel);

	if (c)
	{
		gchar *g, *r;

		VW_output(c->vw, T_NAMES, "sF--", c->server, "%s names: %s", c->Name, list);

		if (tokenize(list))
		{
			while (tok_next(&g, &r)) Channel_Add_User(c, g);
			Channel_Add_User(c, g);
		}

		while (GTK_CLIST(c->vw->Members)->freeze_count) gtk_clist_thaw(GTK_CLIST(c->vw->Members));
	}
	else VW_output(s->vw_active, T_NAMES, "sF--", s, "%s names: %s", channel, list);
}

void Channel_Msg_Join(Server *s, gchar *user, gchar *args)
{
	gchar *channel;
	Channel *c;
	gchar *n, *u;
	gboolean its_me;

	g_return_if_fail(tokenize(args));

	tok_next(&channel, &u);

	c = Channel_Find(s, channel);
	n = nick_part(user);
	u = userhost_part(user);

	its_me = !irc_cmp(n, s->current_nick);

	if (!c && its_me) c = Channel_New(s, channel);

	if (!c) return;

	if (its_me)
	{
		c->State = CHANNEL_JOINED;
		sprintf(channel_tmp, "MODE %s", c->Name);
		Server_Output(s, channel_tmp, FALSE);
	}

	if (c->Prefs & PREF_CHAN_VIEW_JOIN)
	{
		if (u) VW_output(c->vw, T_JOIN, "sF---", c->server, "%s (%s) has joined %s", n, u, c->Name);
		else VW_output(c->vw, T_JOIN, "sF--", "%s (?) has joined %s", user, c->Name);
	}

	Channel_Add_User(c, user);
	VW_Status(c->vw);

	Event_Raise(EV_CHANNEL_JOIN, c->vw, s, c, user, NULL, NULL);
}

void Channel_Msg_Part(Server *s, gchar *user, gchar *channel, gchar *reason)
{
	Channel *c = Channel_Find(s, channel);
	gchar *n;
	
	if (!c) return;

	n = nick_part(user);

	if (reason && *reason == ':') reason += 1;
	if (!reason || !*reason) reason = NULL;

	if (c->Prefs & PREF_CHAN_VIEW_PART)
	{
		if (reason) VW_output(c->vw, T_PART, "sF---", c->server, "%s has left %s (%s)", n, c->Name, reason);
		else VW_output(c->vw, T_PART, "sF--", c->server, "%s has left %s", n, c->Name);
	}

	if (!irc_cmp(n, s->current_nick))
	{
		c->State = CHANNEL_NOT_JOINED;
		Channel_Members_ClearList(c);
		g_free_and_NULL(c->Mode);
	}
	else Channel_Remove_User(c, user);

	VW_Status(c->vw);

	Event_Raise(EV_CHANNEL_PART, c->vw, s, c, user, NULL, reason);
}

void Channel_Msg_Op(Channel *c, gchar *user, gchar *target, gboolean set)
{
	Member *m;
	gint state;

	g_return_if_fail(c);

	if (!(m = Channel_Member_Find(c, target))) return;

	state = m->State;

	if (set) state |= FLAG_OP;
	else state &= ~FLAG_OP;

	Channel_Member_Update(c, m, NULL, state);

	if (set)
		Event_Raise(EV_CHANNEL_OP, c->vw, c->server, c, user, target, NULL);
	else
		Event_Raise(EV_CHANNEL_DEOP, c->vw, c->server, c, user, target, NULL);
}

void Channel_Msg_Voice(Channel *c, gchar *user, gchar *target, gboolean set)
{
	Member *m;
	gint state;

	g_return_if_fail(c);

	if (!(m = Channel_Member_Find(c, target))) return;

	state = m->State;

	if (set) state |= FLAG_VOICE;
	else state &= ~FLAG_VOICE;
	
	Channel_Member_Update(c, m, NULL, state);

	if (set)
		Event_Raise(EV_CHANNEL_VOICE, c->vw, c->server, c, user, target, NULL);
	else
		Event_Raise(EV_CHANNEL_DEVOICE, c->vw, c->server, c, user, target, NULL);
}

void Channel_Msg_Ban(Channel *c, gchar *user, gchar *mask, gboolean set)
{
	g_return_if_fail(c);

	if (set)
		Event_Raise(EV_CHANNEL_BAN, c->vw, c->server, c, user, NULL, mask);
	else
		Event_Raise(EV_CHANNEL_UNBAN, c->vw, c->server, c, user, NULL, mask);
}

void Channel_Msg_Key(Channel *c, gchar *user, gchar *key, gboolean set)
{
	g_return_if_fail(c);

	sprintf(channel_tmp, "%ck %s", (set)? '+' : '-', key);
	Event_Raise(EV_CHANNEL_KEY, c->vw, c->server, c, user, NULL, channel_tmp);
}

void Channel_Msg_Limit(Channel *c, gchar *user, gchar *limit, gboolean set)
{
	g_return_if_fail(c);

	if (c->Limit) sprintf(channel_tmp, "+l %d", c->Limit);
	else sprintf(channel_tmp, "-l");
	Event_Raise(EV_CHANNEL_LIMIT, c->vw, c->server, c, user, NULL, channel_tmp);
}

void Channel_Msg_Flag(Channel *c, gchar *user, gchar flag, gboolean set)
{
	g_return_if_fail(c);

	sprintf(channel_tmp, "%c%c", (set)? '+' : '-', flag);
	Event_Raise(EV_CHANNEL_FLAG, c->vw, c->server, c, user, NULL, channel_tmp);
}

void Channel_Msg_Privmsg(Server *s, gchar *user, gchar *channel, gchar *msg)
{
	Channel *c = Channel_Find(s, channel);
	gchar *n, *u;

	if (!c) return;

	Channels_Member_Userhost_Store(s, user);

	n = nick_part(user);
	u = userhost_part(user);

	if (*msg == ':') msg++;

	VW_output(c->vw, T_CHAN_MSG, "snudt", s, n, u, c->Name, msg);
	Event_Raise(EV_CHANNEL_PRIVMSG, c->vw, s, c, user, NULL, msg);
}
 
void Channel_Msg_Notice(Server *s, gchar *user, gchar *channel, gchar *msg)
{
	Channel *c = Channel_Find(s, channel);
	gchar *n, *u;

	if (!c) return;

	Channels_Member_Userhost_Store(s, user);

	n = nick_part(user);
	u = userhost_part(user);
	if (*msg==':') msg++;

	VW_output(c->vw, T_CHAN_NOTICE, "snudt", s, n, u, c->Name, msg);

	Event_Raise(EV_CHANNEL_NOTICE, c->vw, s, c, user, NULL, msg);
}

/* vi: set ts=3: */

