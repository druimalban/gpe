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
#include "queries.h"
#include "windows.h"
#include "misc.h"
#include "servers.h"
#include "network.h"
#include "prefs.h"
#include "histories.h"
#include "numerics.h"

gchar query_tmp[2048];

/* ------------------------------------------------------------------------------------ */

void Query_Input(gpointer data, gchar *text)
{
	Query *q = (Query *) data;

	if (*text) sprintf(query_tmp,"PRIVMSG %s :%s", q->member.nick, text);
	else sprintf(query_tmp,"PRIVMSG %s : ", q->member.nick);
	if (!Server_Output(q->server, query_tmp, TRUE)) return;
	VW_output(q->vw, T_OWN_MSG, "snudt", q->server, q->server->current_nick, "userhost", q->member.nick, text);
}

void Query_Close(gpointer data)
{
	Query *q = (Query *) data;

	if (q->vw->rw->vw_active == q->vw)
		if (!GW_Activate_Last_VW(q->vw->rw)) VW_Activate(q->server->vw);

	q->server->Queries = g_list_remove(q->server->Queries, q);
	VW_Remove_From_GW(q->vw, FALSE);

	g_free(q->member.nick);
	g_free(q->member.userhost);
	g_free(q);
}

void Query_Infos(gpointer data)
{
	Query *q = (Query *) data;

	gtk_label_set((GtkLabel *) q->vw->WFL[0], q->server->fs->Name);

	sprintf(query_tmp,"Query with %s", q->member.nick);
	gtk_label_set((GtkLabel *) q->vw->WFL[1], query_tmp);

	if (q->member.userhost)
	{
		gtk_label_set((GtkLabel *) q->vw->WFL[2], q->member.userhost);
		gtk_widget_show(q->vw->WF[2]);
	}
	else gtk_widget_hide(q->vw->WF[2]);
}

/* ------------------------------------------------------------------------------------ */

Query *Query_Find(Server *s, gchar *user)
{
	GList *l;
	gchar *t;

	g_return_val_if_fail(s, NULL);
	g_return_val_if_fail(user, NULL);

	t = nick_part(user);
	l = s->Queries;

	while (l)
	{
		if (!irc_cmp(t, ((Query *) l->data)->member.nick))
		{
			Query *q = (Query *) l->data;
			
			if (!(q->member.userhost))
				if ((q->member.userhost = g_strdup(userhost_part(user)))) VW_Status(q->vw);

			return q;
		}
		l = l->next;
	}

	return NULL;
}

gboolean Query_who_reply(struct Message *m, gpointer data)
{
	if (m)
	{
		sprintf(query_tmp, "%s!%s@%s", m->args[5], m->args[2], m->args[3]);
		Query_Find(m->server, query_tmp);
	}

	return FALSE;
}

Query *Query_New(Server *s, gchar *partner)
{
	static guint32 qu_id = 0;

	Query *q = Query_Find(s, partner);

	if (!q)
	{
		q = (Query *) g_malloc0(sizeof(struct Query));

		q->object_id = qu_id++;

		q->member.nick = g_strdup(nick_part(partner));
		q->member.userhost = g_strdup(userhost_part(partner));
		q->server = s;
		strcpy(query_tmp, q->member.nick);

		q->vw = VW_new();
		q->vw->pmask.w_type = W_QUERY;
		q->vw->Name = q->member.nick;
		q->vw->Title = q->member.nick;
		q->vw->Resource = (gpointer) q;
		q->vw->Infos = Query_Infos;
		q->vw->Close = Query_Close;
		q->vw->Input = Query_Input;

		VW_Init(q->vw, s->vw->rw, 3);

		s->Queries = g_list_append(s->Queries, q);
	}

	if (!q->member.userhost)
	{
		if (Server_Callback_Add(q->server, RPL_WHOREPLY, q->member.nick, CBF_QUIET, Query_who_reply, NULL));
		{
			sprintf(query_tmp, "WHO %s", q->member.nick);
			Server_Output(q->server, query_tmp, FALSE);
		}
	}
	
	/* <PREFS> Do we have to activate a new query ? */
	if (!*(gtk_entry_get_text((GtkEntry *) q->vw->rw->vw_active->entry))) VW_Activate(q->vw);

	return q;
}

void Query_Msg(Server *s, gchar *partner, gchar *msg)
{
	Query *q = Query_Find(s, partner);

	if (!q) q = Query_New(s, partner);

	VW_output(q->vw, T_PRIV_MSG, "snudt", s, q->member.nick, q->member.userhost, s->current_nick, msg+1);
}

void Query_Notice(Server *s, gchar *partner, gchar *msg)
{
	Query *q = Query_Find(s, partner);

	if (!q) q = Query_New(s, partner);

	VW_output(q->vw, T_PRIV_NOTICE, "snudt", s, q->member.nick, q->member.userhost, s->current_nick, msg+1);
}

/* vi: set ts=3: */

