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

/* ----- Parsing of IRC messages from servers --------------------------------------------------------- */

#include "olirc.h"
#include "parse_msg.h"
#include "network.h"
#include "windows.h"
#include "channels.h"
#include "queries.h"
#include "misc.h"
#include "servers.h"
#include "ctcp.h"
#include "servers_dialogs.h"
#include "numerics.h"
#include "ignores.h"
#include "scripting.h"

#include <string.h>

gchar pmsg_msg[2048];
gchar pmsg_tmp[2048];
gchar pmsg_tmp2[1024];

#define MSG_HANDLER(f) extern void f(struct Message *m)

/* ----- Messages handlers ------------------------------------------------------------ */

MSG_HANDLER(Msg_Debug)
{	
	int i;
	fprintf(stdout, "\n");
	fprintf(stdout, "c_num = %d\n", m->c_num);
	fprintf(stdout, "command = %s\n", m->command);
	fprintf(stdout, "prefix = %s\n", m->prefix);
	for (i=0; i<m->nargs; i++) fprintf(stdout, "arg[%d] = %s\n", i, m->args[i]);
}

MSG_HANDLER(Msg_Dump_Server)
{
	/* Dumps the message in the server window */

	VW_output(m->server->vw, T_ANSWER, "sF-", m->server, "%s", m->args[1]+1);

	if (m->c_num == RPL_WELCOME) Server_Init(m->server, m->args[0]);
}

MSG_HANDLER(Msg_Dump)
{
	/* Dumps the message in the active window */

	VW_output(m->server->vw_active, T_ANSWER, "sF-", m->server, "%s", m->args[1]+1);
}

MSG_HANDLER(Msg_Kick)
{
	Channel *c = Channel_Find(m->server, m->args[0]);
	gboolean its_me = !irc_cmp(m->args[1], m->server->current_nick);

	Channels_Member_Userhost_Store(m->server, m->prefix);

	if (c)
	{
		if (its_me || c->Prefs & PREF_CHAN_VIEW_KICK)
		{
			if (its_me && m->nargs>2 ) VW_output(c->vw, T_KICK, "sF--", m->server, "YOU have been kicked by %s (%s)", m->nick, m->args[2]+1);
			else if (its_me) VW_output(c->vw, T_KICK, "sF-", m->server, "YOU have been kicked by %s.", m->nick);
			else if (m->nargs>2) VW_output(c->vw, T_KICK, "sF--", m->server, "%s has been kicked by %s (%s)", m->args[1], m->nick, m->args[2]+1);
			else VW_output(c->vw, T_KICK, "sF-", m->server, "%s has been kicked by %s.", m->args[1], m->nick);
		}

		if (its_me)
		{
			c->State = CHANNEL_KICKED;
			Channel_Members_ClearList(c);
			g_free_and_NULL(c->Mode);
			VW_Status(c->vw);
		}
		else Channel_Remove_User(c, m->args[1]);
	}
}

MSG_HANDLER(Msg_Ping)
{
	sprintf(pmsg_msg,"PONG %s", m->args[0]);
	Server_Output(m->server,pmsg_msg, FALSE);
	/* <PREFS> Display "Ping ? Pong !" messages in server window : on | off */
}

MSG_HANDLER(Msg_Join)
{
	/* Some servers send JOIN message with a string beginning by \007 appened
	   after the channel name. This has a special meaning on some irc networks.
		For now, the channel name is cut at the first \007 encountred. */

	gchar *seek = strchr(m->args[0], 7); if (seek) *seek = 0;

	if (m->args[0][0]==':') Channel_Msg_Join(m->server, m->prefix, m->args[0]+1);
	else Channel_Msg_Join(m->server, m->prefix, m->args[0]);
}

MSG_HANDLER(Msg_Part)
{
	if (m->nargs>1) Channel_Msg_Part(m->server, m->prefix, m->args[0], m->args[1]);
	else Channel_Msg_Part(m->server, m->prefix, m->args[0], NULL);
}

MSG_HANDLER(Msg_Nick)
{
	Channel *c;
	Query *q;
	GList *l;
	Member *mb;
	gboolean its_me = !irc_cmp(m->nick, m->server->current_nick);
	gchar *new_nick = m->args[0];

	if (*m->args[0]==':') new_nick++;

	if (its_me)
	{
		sprintf(pmsg_msg,"You are now known as %s", new_nick);
		VW_output(m->server->vw, T_NICK, "#sF-", m->server, "%s", pmsg_msg);
	}
	else sprintf(pmsg_msg,"%s is now known as %s", m->nick, new_nick);

	l = m->server->Channels;
	while (l)
	{
		c = (Channel *) l->data;

		if ((mb = Channel_Member_Find(c, m->prefix)))
		{
			if (its_me) VW_output(c->vw, T_NICK, "#sF-", m->server, "%s", pmsg_msg);
	 		else if (c->Prefs & PREF_CHAN_VIEW_NICK) VW_output(c->vw, T_NICK, "sF-", m->server, "%s", pmsg_msg);

			Channel_Member_Update(c, mb, new_nick, mb->State);
		}

		l = l->next;
	}

	/* If an old query with the new nick is still open, we close it */
	if (irc_cmp(m->nick, new_nick) && (q = Query_Find(m->server, new_nick))) VW_Close(q->vw);

	l = m->server->Queries;
	while (l)
	{
		q = (Query *) l->data;
		if (!irc_cmp(q->member.nick, m->nick))
		{
			VW_output(q->vw, T_NICK, "sF-", m->server, "%s", pmsg_msg);
			g_free_and_set(q->member.nick, g_strdup(new_nick));
			q->vw->Title = q->member.nick;
			q->vw->Name = q->member.nick;
			gtk_label_set((GtkLabel *) q->vw->Page_Tab_Label, q->member.nick);
			VW_Status(q->vw);
			break;
		}
		else if (its_me) VW_output(q->vw, T_NICK, "#sF-", m->server, "%s", pmsg_msg);
		l = l->next;
	}

	if (its_me)
	{
		g_free_and_set(m->server->current_nick, g_strdup(new_nick));
		VW_Status(m->server->vw);
	}
}

MSG_HANDLER(Msg_Quit)
{
	GList *l;
	Channel *c;
	Query *q;
	gboolean displayed = FALSE;

	/* TODO Check the next line : sometimes Ol-Irc displays : "foobar has quit irc ((null))." */
	if (m->nargs) sprintf(pmsg_msg, "%s has quit irc (%s).", m->nick, m->args[0]+1);
	else sprintf(pmsg_msg, "%s has quit irc.", m->nick);

	l = m->server->Channels;
	while (l)
	{
		c = (Channel *) l->data;
		if (Channel_Remove_User(c, m->nick))
		{
			if (c->Prefs&PREF_CHAN_VIEW_QUIT) VW_output(c->vw, T_QUIT, "sF-", m->server, "%s", pmsg_msg);
			displayed = TRUE;
		}
		l = l->next;
	}

	l = m->server->Queries;
	while (l)
	{
		q = (Query *) l->data;
		if (!(irc_cmp(q->member.nick, m->nick))) VW_output(q->vw, T_QUIT, "sF-", m->server, "%s", pmsg_msg);
		l = l->next;
	}

	if (!displayed) VW_output(m->server->vw, T_NORMAL, "#sF-", m->server, "%s", pmsg_msg);
}

MSG_HANDLER(Msg_Invite)
{
	gchar *c;
	if (m->nargs<2) return;
	if (Ignore_Check(m->server, m->prefix, IGNORE_INVITE)) return;
	c = m->args[1];
	if (*c == ':') c++;
	if (!irc_cmp(m->server->current_nick, m->args[0])) VW_output(m->server->vw_active, T_INVITE, "sF--", m->server, "%s invites you to join channel %s", m->nick, c);
	else VW_output(m->server->vw_active, T_INVITE, "sF--", m->server, "%s invites %s to join channel %s", m->nick, m->args[0], c);
}

MSG_HANDLER(Msg_Topic)
{
	Channel *c = Channel_Find(m->server, m->args[m->nargs-2]);

	if (!(m->c_num)) Channels_Member_Userhost_Store(m->server, m->prefix);

	if (c)
	{
		g_free_and_NULL(c->Topic);
		if (m->c_num == RPL_NOTOPIC)
		{
			sprintf(pmsg_msg,"%s: %s",m->args[m->nargs-2], m->args[m->nargs-1]+1);
		}
		else if (m->c_num == RPL_TOPIC)
		{
			c->Topic = g_strdup(m->args[m->nargs-1]+1);
			sprintf(pmsg_msg,"%s topic is: %s", m->args[m->nargs-2], c->Topic);
		}
		else
		{
			c->Topic = g_strdup(m->args[m->nargs-1]+1);
			sprintf(pmsg_msg, "%s changed topic to: %s", m->nick, c->Topic);
		}
		if (c->Prefs & PREF_CHAN_VIEW_TOPIC) VW_output(c->vw, T_TOPIC, "sF-", m->server, "%s", pmsg_msg);
		VW_Status(c->vw);
	}
}

MSG_HANDLER(Msg_Mode)
{
	gint d, i;
	gboolean set = FALSE, chanmodeset;
	Channel *c = NULL;

	chanmodeset = !(m->c_num);

	if (chanmodeset)
	{
		d = 0;
		Channels_Member_Userhost_Store(m->server, m->prefix);
		sprintf(pmsg_msg,"%s set mode", m->nick);
	}
	else
	{
		d = 1;
		sprintf(pmsg_msg,"%s mode is", m->args[d]);
	}
	
	for (i = d+1; i < m->nargs; i++)
		if (*m->args[i]==':') strspacecat(pmsg_msg, m->args[i]+1);
		else strspacecat(pmsg_msg, m->args[i]);

	if (chanmodeset)
	{
		strcat(pmsg_msg, " on ");
		strcat(pmsg_msg, m->args[d]);
	}

	if (IsChannelName(m->args[d])) c = Channel_Find(m->server, m->args[d]);

	if (c)
	{
		gint j, k;
		gboolean flag = FALSE;

		if (c->Prefs & PREF_CHAN_VIEW_MODE) VW_output(c->vw, T_MODE, "sF-", m->server, "%s", pmsg_msg);

		if (chanmodeset && c->Mode) strcpy(pmsg_tmp2, c->Mode);
		else { *pmsg_tmp2 = 0; flag = !chanmodeset; }

		for (i = d+1; i < m->nargs; i++)
		{
			k = i;
			
			for (j = 0; j < strlen(m->args[k]); j++)
			{
				switch (m->args[k][j])
				{
					case '+': set = TRUE; break;
					case '-': set = FALSE; break;

					case 'b':
						i++;
						sprintf(pmsg_msg, "%cb %s", (set)? '+' : '-', m->args[i]);
						Event_Raise(EV_CHANNEL_MODE, c->vw, c->server, c, m->prefix, NULL, pmsg_msg);
						Channel_Msg_Ban(c, m->prefix, m->args[i], set);
						break;
					
					case 'v':
						i++;
						sprintf(pmsg_msg, "%cv %s", (set)? '+' : '-', m->args[i]);
						Event_Raise(EV_CHANNEL_MODE, c->vw, c->server, c, m->prefix, NULL, pmsg_msg);
						Channel_Msg_Voice(c, m->prefix, m->args[i], set);
						break;
					
					case 'o':
						i++;
						sprintf(pmsg_msg, "%co %s", (set)? '+' : '-', m->args[i]);
						Event_Raise(EV_CHANNEL_MODE, c->vw, c->server, c, m->prefix, NULL, pmsg_msg);
						Channel_Msg_Op(c, m->prefix, m->args[i], set);
						break;

					case 'k':
						i++;
						flag = TRUE;
						g_free_and_NULL(c->Key);
						if (set) c->Key = g_strdup(m->args[i]);
						sprintf(pmsg_msg, "%ck %s", (set)? '+' : '-', m->args[i]);
						Event_Raise(EV_CHANNEL_MODE, c->vw, c->server, c, m->prefix, NULL, pmsg_msg);
						if (chanmodeset) Channel_Msg_Key(c, m->prefix, m->args[i], set);
						break;
					
					case 'l':
						flag = TRUE;
						if (set)
						{
							c->Limit = atoi(m->args[++i]);
							sprintf(pmsg_msg, "+l %d", c->Limit);
						}
						else
						{
							c->Limit = 0;
							sprintf(pmsg_msg, "-l");
						}
						Event_Raise(EV_CHANNEL_MODE, c->vw, c->server, c, m->prefix, NULL, pmsg_msg);
						if (chanmodeset) Channel_Msg_Limit(c, m->prefix, (set)? m->args[i] : NULL, set);
						break;
					
					case 'I': case 'e':
						i++;
						sprintf(pmsg_msg, "%c%c %s", (set)? '+' : '-', m->args[k][j], m->args[i]);
						Event_Raise(EV_CHANNEL_MODE, c->vw, c->server, c, m->prefix, NULL, pmsg_msg);
						break;

					default:
					{
						gchar *s = pmsg_tmp2;
						flag = TRUE;
						if (chanmodeset)
						{
							sprintf(pmsg_msg, "%c%c", (set)? '+' : '-', m->args[k][j]);
							Event_Raise(EV_CHANNEL_MODE, c->vw, c->server, c, m->prefix, NULL, pmsg_msg);
							Channel_Msg_Flag(c, m->prefix, m->args[k][j], set);
							while (*s && *s != m->args[k][j]) s++;
							if (set && !*s)
							{
								*s++ = m->args[k][j];
								*s = 0;
							}
							else if (!set && *s) *s = ' ';
						}
						else
						{
							s += strlen(pmsg_tmp2);
							*s++ = m->args[k][j];
							*s = 0;
						}
					}
				}
			}
		}

		if (flag)
		{
			gchar *r, *w;
			r = w = pmsg_tmp2;
			while (*r) { if (*r != ' ') *w++ = *r; r++; }
			*w = 0;
			g_free_and_set(c->Mode, g_strdup(pmsg_tmp2));
			VW_Status(c->vw);
		}
	}
	else VW_output(m->server->vw, T_MODE, "sF-", m->server, "%s", pmsg_msg);

}

MSG_HANDLER(Msg_Privmsg)
{
	gint i;

	for (i=0; i<m->nargs-1; i++)
	{
#ifdef USE_CTCP
		if (Is_CTCP(m->args[m->nargs-1]))
		{
			CTCP_Parse(m->server, m->prefix, m->args[i], FALSE, m->args[m->nargs-1]+1);
			continue;
		}
#endif
		if (IsChannelName(m->args[i]))
		{
			if (Ignore_Check(m->server, m->prefix, IGNORE_CHANNEL)) return;
			Channel_Msg_Privmsg(m->server, m->prefix, m->args[i], m->args[m->nargs-1]);
		}
		else if (!irc_cmp(m->args[0], m->server->current_nick))
		{
			if (Ignore_Check(m->server, m->prefix, IGNORE_PRIVATE)) return;
			Query_Msg(m->server, m->prefix, m->args[m->nargs-1]);
		}
		else
		{
			VW_output(m->server->vw, T_ERROR, "sF---", m->server, "PRIVMSG from %s to %s : %s", m->prefix, m->args[i], m->args[m->nargs-1]);
		}
	}
}

MSG_HANDLER(Msg_Notice)
{
	gint i;
	gboolean server = FALSE;

	if (!m->nick)
	{
		m->nick = g_strdup(m->server->fs->Name);
		server = TRUE;
	}

	for (i=0; i<m->nargs-1; i++) /* XXX ??? What is the purpose of this loop ????? XXX */
	{
#ifdef USE_CTCP
		if (Is_CTCP(m->args[m->nargs-1]))
		{
			CTCP_Parse(m->server, m->prefix, m->args[i], TRUE, m->args[m->nargs-1]+1);
			continue;
		}
#endif
		if (IsChannelName(m->args[i]))
		{
			if (Ignore_Check(m->server, m->prefix, IGNORE_CHANNEL)) return;
			Channel_Msg_Notice(m->server, m->prefix, m->args[i], m->args[m->nargs-1]);
		}
		else
		{
			if (!server && Ignore_Check(m->server, m->prefix, IGNORE_NOTICE)) return;
			/* <PREFS> Server notices : in server window | in active window */
			if (server || strchr(m->nick, '.')) VW_output(m->server->vw, T_PRIV_NOTICE, "snudt", m->server, m->nick, m->userhost, m->args[i], m->args[m->nargs-1]+1);
			else VW_output(m->server->vw_active, T_PRIV_NOTICE, "snudt", m->server, m->nick, m->userhost, m->args[i], m->args[m->nargs-1]+1);
		}
	}
}

/* - - - Replies messages - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

MSG_HANDLER(Msg_MYINFO)
{
	VW_output(m->server->vw, T_ANSWER, "sF----", m->server, "%s: version %s, user modes %s, channel modes %s", m->args[1], m->args[2], m->args[3], m->args[4]);
}

MSG_HANDLER(Msg_AWAY)
{
	VW_output(m->server->vw_active, T_ANSWER, "sF---", m->server, "%s %s away (%s)", m->args[1], (m->server->Receiving==SERVER_WHOWAS)? "was":"is currently", m->args[2]+1);
}

MSG_HANDLER(Msg_ISON)
{
	VW_output(m->server->vw_active, T_ANSWER, "sF-", m->server, "Currently on IRC: %s", m->args[1]+1);
}

MSG_HANDLER(Msg_AWAY_Status)
{
	VW_output(m->server->vw_active, T_ANSWER, "sF-", m->server, "%s", m->args[1]+1);
}

MSG_HANDLER(Msg_WHOISUSER)
{
	m->server->Receiving = SERVER_WHOIS;
	VW_output(m->server->vw_active, T_ANSWER, "sF----", m->server, "%s is %s@%s : %s", m->args[1], m->args[2], m->args[3], m->args[5]+1);
}

MSG_HANDLER(Msg_WHOISSERVER)
{
	VW_output(m->server->vw_active, T_ANSWER, "sF----", m->server, "%s %s using server %s (%s)", m->args[1], (m->server->Receiving==SERVER_WHOWAS)? "was":"is", m->args[2], m->args[3]+1);
}

MSG_HANDLER(Msg_WHOISOPERATOR)
{
	VW_output(m->server->vw_active, T_ANSWER, "sF--", m->server, "%s %s", m->args[1], m->args[2]+1);
}

MSG_HANDLER(Msg_WHOWASUSER)
{
	m->server->Receiving = SERVER_WHOWAS;
	VW_output(m->server->vw_active, T_ANSWER, "sF----", m->server, "%s was %s@%s : %s", m->args[1], m->args[2], m->args[3], m->args[5]+1);
}

MSG_HANDLER(Msg_WHOISIDLE)
{
	gint idle = atol(m->args[2]);
	gchar *d = (*(m->args[3])==':')? NULL : irc_date((time_t) atol(m->args[3]));
	if (d) sprintf(pmsg_msg, "%s signed on on %s, and has been idle for %s", m->args[1], d, Duration(idle));
	else sprintf(pmsg_msg, "%s has been idle for %s", m->args[1], Duration(idle));
	VW_output(m->server->vw_active, T_ANSWER, "sF-", m->server, "%s", pmsg_msg);
}

MSG_HANDLER(Msg_WHOISCHANNELS)
{
	VW_output(m->server->vw_active, T_ANSWER, "sF--", m->server, "%s is on channels: %s", m->args[1], m->args[2]+1);
}

MSG_HANDLER(Msg_TOPICWHOTIME)
{
	Channel *c = Channel_Find(m->server, m->args[1]);
	if (m->args[3]) VW_output((c)? c->vw : m->server->vw_active, T_ANSWER, "sF---", m->server, "%s topic was set by %s on %s", m->args[1], m->args[2], irc_date((time_t) atol(m->args[3])));
	else VW_output((c)? c->vw : m->server->vw_active, T_ANSWER, "sF--", m->server, "%s topic was set by %s", m->args[1], m->args[2]);
}

MSG_HANDLER(Msg_CREATIONTIME)
{
	Channel *c = Channel_Find(m->server, m->args[1]);
	VW_output((c)? c->vw : m->server->vw_active, T_ANSWER, "sF--", m->server, "%s was created on %s", m->args[1], irc_date((time_t) atol(m->args[2])));
}

MSG_HANDLER(Msg_NAMREPLY)
{
	m->server->Receiving = SERVER_NAMES;
	Channel_Msg_Names(m->server, m->args[m->nargs-2], m->args[m->nargs-1]+1);
}

MSG_HANDLER(Msg_LINKS)
{
	m->server->Receiving = SERVER_LINKS;
	VW_output(m->server->vw_active, T_ANSWER, "sF---", m->server, "%s %s %s", m->args[1], m->args[2], m->args[3]+1);
}

MSG_HANDLER(Msg_INFOSTART)
{
	m->server->Receiving = SERVER_INFO;
	Msg_Dump_Server(m);
}

MSG_HANDLER(Msg_MOTDSTART)
{
	m->server->Receiving = SERVER_MOTD;
	if (m->server->fs->Flags & FSERVER_HIDEMOTD) return;
	VW_output(m->server->vw, T_ANSWER, "sF-", m->server, "%s", m->args[m->nargs-1]+1);
}

MSG_HANDLER(Msg_MOTD)
{
	if (m->server->fs->Flags & FSERVER_HIDEMOTD) return;
	VW_output(m->server->vw, T_ANSWER, "sF-", m->server, "%s", m->args[m->nargs-1]+1);
}

MSG_HANDLER(Msg_INFO)
{
	VW_output(m->server->vw, T_ANSWER, "sF-", m->server, "%s", m->args[m->nargs-1]+1);
}

MSG_HANDLER(Msg_LUSERS_Various)
{
	VW_output(m->server->vw, T_ANSWER, "sF-", m->server, "%d %s ", atoi(m->args[1]), m->args[2]+1);
}

MSG_HANDLER(Msg_LIST)
{
	gint i;
	m->server->Receiving = SERVER_LIST;
	sprintf(pmsg_msg, "(%d)", m->c_num);
	for (i = 0; i<m->nargs; i++) strspacecat(pmsg_msg, m->args[i]);
	VW_output(m->server->vw_active, T_ANSWER, "sF-", m->server, "%s", pmsg_msg);
}

MSG_HANDLER(Msg_BANLIST)
{
	m->server->Receiving = SERVER_BANLIST;
	VW_output(m->server->vw_active, T_ANSWER, "sF-", m->server, "%s %s", m->args[1], m->args[2]);
}

MSG_HANDLER(Msg_INVITELIST)
{
	m->server->Receiving = SERVER_INVITELIST;
	VW_output(m->server->vw_active, T_ANSWER, "sF-", m->server, "%s %s", m->args[1], m->args[2]);
}

MSG_HANDLER(Msg_EXCEPTLIST)
{
	m->server->Receiving = SERVER_EXCEPTLIST;
	VW_output(m->server->vw_active, T_ANSWER, "sF-", m->server, "%s %s", m->args[1], m->args[2]);
}

MSG_HANDLER(Msg_INVITING)
{
	VW_output(m->server->vw_active, T_ANSWER, "sF-", m->server, "%s has been invited to %s", m->args[1], m->args[2]);
}

MSG_HANDLER(Msg_WHOREPLY)
{
	gint i;
	gpointer sc;

	m->server->Receiving = SERVER_WHO;

	sprintf(pmsg_msg, "%s!%s@%s", m->args[5], m->args[2], m->args[3]);
	Channels_Member_Userhost_Store(m->server, pmsg_msg);

	if ((sc = Server_Callback_Check(m->server, RPL_WHOREPLY, m->args[5], NULL)))
	{
		m->server->Flags |= SERVER_HIDE;
		if (!Server_Callback_Call(sc, m)) return;
	}

	sprintf(pmsg_msg, "(%d)", m->c_num);
	for (i = 0; i<m->nargs; i++) strspacecat(pmsg_msg, m->args[i]);
	VW_output(m->server->vw_active, T_ANSWER, "sF-", m->server, "%s", pmsg_msg);
}

/* - - - 'EndOf' messages - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

MSG_HANDLER(Msg_ENDOFMOTD)
{
	m->server->Receiving = SERVER_NONE;
	if (m->server->Flags & SERVER_HIDE) { m->server->Flags &= ~SERVER_HIDE; return; }
	if (m->server->fs->Flags & FSERVER_HIDEMOTD) return;
	/* <PREFS> Display ENDOF MOTD message : on | off */
	VW_output(m->server->vw, T_ANSWER, "sF-", m->server, "%s", m->args[1]+1);
}

MSG_HANDLER(Msg_ENDOFWHO)
{
	gpointer sc;

	m->server->Receiving = SERVER_NONE;

	if ((sc = Server_Callback_Check(m->server, RPL_WHOREPLY, m->args[1], NULL)))
		if (!Server_Callback_Call(sc, NULL)) return;

	if (m->server->Flags & SERVER_HIDE) { m->server->Flags &= ~SERVER_HIDE; return; }

	/* <PREFS> Display ENDOF messages : on | off */
	VW_output(m->server->vw_active, T_ANSWER, "sF--", m->server, "%s: %s", m->args[1], m->args[2]+1);
}

MSG_HANDLER(Msg_ENDOFWHOIS)
{
	m->server->Receiving = SERVER_NONE;
	if (m->server->Flags & SERVER_HIDE) { m->server->Flags &= ~SERVER_HIDE; return; }
	/* <PREFS> Display ENDOF messages : on | off */
	VW_output(m->server->vw_active, T_ANSWER, "sF--", m->server, "%s: %s", m->args[1], m->args[2]+1);
}

MSG_HANDLER(Msg_ENDOFINFO)
{
	m->server->Receiving = SERVER_NONE;
	if (m->server->Flags & SERVER_HIDE) { m->server->Flags &= ~SERVER_HIDE; return; }
	/* <PREFS> Display ENDOF messages : on | off */
	VW_output(m->server->vw_active, T_ANSWER, "sF-", m->server, "%s", m->args[1]+1);
}

MSG_HANDLER(Msg_ENDOFLINKS)
{
	m->server->Receiving = SERVER_NONE;
	if (m->server->Flags & SERVER_HIDE) { m->server->Flags &= ~SERVER_HIDE; return; }
	/* <PREFS> Display ENDOF messages : on | off */
	VW_output(m->server->vw_active, T_ANSWER, "sF--", m->server, "%s: %s", m->args[1], m->args[2]+1);
}

MSG_HANDLER(Msg_ENDOFNAMES)
{
	Channel *c = Channel_Find(m->server, m->args[1]);
	m->server->Receiving = SERVER_NONE;
	if (m->server->Flags & SERVER_HIDE) { m->server->Flags &= ~SERVER_HIDE; return; }
	/* <PREFS> Display ENDOF messages : on | off */
	VW_output((c)? c->vw : m->server->vw_active, T_NAMES, "sF--", m->server, "%s: %s", m->args[1], m->args[2]+1);
}

MSG_HANDLER(Msg_ENDOFBANLIST)
{
	m->server->Receiving = SERVER_NONE;
	if (m->server->Flags & SERVER_HIDE) { m->server->Flags &= ~SERVER_HIDE; return; }
	/* <PREFS> Display ENDOF messages : on | off */
	VW_output(m->server->vw_active, T_ANSWER, "sF--", m->server, "%s: %s", m->args[1], m->args[2]+1);
}

MSG_HANDLER(Msg_ENDOFINVITELIST)
{
	m->server->Receiving = SERVER_NONE;
	if (m->server->Flags & SERVER_HIDE) { m->server->Flags &= ~SERVER_HIDE; return; }
	/* <PREFS> Display ENDOF messages : on | off */
	VW_output(m->server->vw_active, T_ANSWER, "sF--", m->server, "%s: %s", m->args[1], m->args[2]+1);
}

MSG_HANDLER(Msg_ENDOFEXCEPTLIST)
{
	m->server->Receiving = SERVER_NONE;
	if (m->server->Flags & SERVER_HIDE) { m->server->Flags &= ~SERVER_HIDE; return; }
	/* <PREFS> Display ENDOF messages : on | off */
	VW_output(m->server->vw_active, T_ANSWER, "sF--", m->server, "%s: %s", m->args[1], m->args[2]+1);
}

MSG_HANDLER(Msg_ENDOFWHOWAS)
{
	m->server->Receiving = SERVER_NONE;
	if (m->server->Flags & SERVER_HIDE) { m->server->Flags &= ~SERVER_HIDE; return; }
	/* <PREFS> Display ENDOF messages : on | off */
	VW_output(m->server->vw_active, T_ANSWER, "sF--", m->server, "%s: %s", m->args[1], m->args[2]+1);
}

MSG_HANDLER(Msg_LISTEND)
{
	m->server->Receiving = SERVER_NONE;
	if (m->server->Flags & SERVER_HIDE) { m->server->Flags &= ~SERVER_HIDE; return; }
	/* <PREFS> Display ENDOF messages : on | off */
	VW_output(m->server->vw_active, T_ANSWER, "sF-", m->server, "%s", m->args[1]+1);
}

/* - - - Fatal error messages - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

MSG_HANDLER(Msg_Error)
{
	VW_output(m->server->vw, T_ERROR, "sF-", m->server, "ERROR: %s", m->args[0]+1);
	VW_Activate(m->server->vw);
}

/* - - - Numerical error messages - - - - - - - - - - - - - - - - - - - - - - - - - - - */

MSG_HANDLER(Msg_ERR_BANNEDFROMCHAN)
{
	Channel *c = Channel_Find(m->server, m->args[1]);
	Virtual_Window *vw = m->server->vw_active;

	if (c) { c->State = CHANNEL_BANNED; VW_Status(c->vw); vw = c->vw; }

	VW_output(vw, T_WARNING, "sF-", c->server, "Cannot join %s: You are banned from this channel", m->args[1]);
}

MSG_HANDLER(Msg_ERR_INVITEONLYCHAN)
{
	Channel *c = Channel_Find(m->server, m->args[1]);
	Virtual_Window *vw = m->server->vw_active;

	if (c) { c->State = CHANNEL_INVITE_ONLY; VW_Status(c->vw); vw = c->vw; }

	VW_output(vw, T_WARNING, "sF-", c->server, "Cannot join %s: Channel is invite only", m->args[1]);
}

MSG_HANDLER(Msg_ERR_CHANNELISFULL)
{
	Channel *c = Channel_Find(m->server, m->args[1]);
	Virtual_Window *vw = m->server->vw_active;

	if (c) { c->State = CHANNEL_FULL; VW_Status(c->vw); vw = c->vw; }

	VW_output(vw, T_WARNING, "sF-", c->server, "Cannot join %s: Channel is full", m->args[1]);

	/* <PREFS> Try to rejoin the channel automatically every x seconds, but no more
				  than y tries ? */
}

MSG_HANDLER(Msg_ERR_BADCHANNELKEY)
{
	Channel *c = Channel_Find(m->server, m->args[1]);
	Virtual_Window *vw = m->server->vw_active;

	if (c) { c->State = CHANNEL_BAD_KEY; VW_Status(c->vw); vw = c->vw; }

	VW_output(vw, T_WARNING, "sF-", c->server, "Cannot join %s: You need the right key", m->args[1]);

	/* <PREFS> Try to use last key, if one is known ? Yes | No */

	if (c->Key)	/* Try to use the last known key */
	{
		VW_output(vw, T_WARNING, "sF-", c->server, "%s: Trying last known key '%s'...", c->Name, c->Key);
		Channel_Join(c, c->Key);
		g_free_and_NULL(c->Key);
	}
}

MSG_HANDLER(Msg_ERR_TOOMANYCHANNELS)
{
	Channel *c = Channel_Find(m->server, m->args[1]);
	Virtual_Window *vw = m->server->vw_active;

	if (c)
	{
		c->State = CHANNEL_TOOMANYCHANS;
		VW_Status((vw = c->vw));

		/* <PREFS> Try to rejoin the channel next time another channel is closed or parted ?
				(Not when kicked or banned from the other channel) */
	}

	VW_output(vw, T_WARNING, "sF-", m->server, "Cannot join %s: You have joined too many channels", m->args[1]);
}

MSG_HANDLER(Msg_ERR_UNAVAILRESOURCE)
{
	Virtual_Window *vw = m->server->vw_active;

	if (!m->server->current_nick)
	{
		strcpy(pmsg_msg, "This nickname is temporarily unavailable.\nPlease choose another nickname :");
		dialog_server_nick(m->server, pmsg_msg, m->server->fs->Nick);
	}
	else if (IsChannelName(m->args[1]))
	{
		Channel *c = Channel_Find(m->server, m->args[1]);
		if (c) { c->State = CHANNEL_UNAVAILABLE; VW_Status(c->vw); vw = c->vw; }
		VW_output(vw, T_WARNING, "sF-", c->server, "Cannot join %s: Channel is temporarily unavailable.", m->args[1]);

	/* <PREFS> Try to rejoin the channel automatically every x seconds, but no more
				  than y tries ? */
	}
	else VW_output(vw, T_WARNING, "sF-", m->server, "%s: Temporarily unavailable.", m->args[1]);
}

/* ----- Messages tables --------------------------------------------------------------- */

struct { char *Command; void (* Function)(struct Message *); } IRC_Messages[] =
{
	{ "PRIVMSG", Msg_Privmsg },
	{ "NOTICE", Msg_Notice },
	{ "JOIN", Msg_Join },
	{ "PART", Msg_Part },
	{ "PING", Msg_Ping },
	{ "QUIT", Msg_Quit },
	{ "KICK", Msg_Kick },
	{ "MODE", Msg_Mode },
	{ "TOPIC", Msg_Topic },
	{ "ERROR", Msg_Error },
	{ "NICK", Msg_Nick },
	{ "INVITE", Msg_Invite },
/* { "WALLOPS', Msg_Wallops },
	{ "PONG", Msg_Pong }, */
	{ "", NULL }
};

struct { unsigned int Numeric; void (* Function)(struct Message *); } IRC_Numerics[] =
{
	/* Answers numerics */

	{ RPL_WELCOME, Msg_Dump_Server },				/* Ok */
	{ RPL_YOURHOST, Msg_Dump_Server },				/* Ok */
	{ RPL_CREATED, Msg_Dump_Server },				/* Ok */
	{ RPL_MYINFO, Msg_MYINFO },						/* Ok */
	{ RPL_LUSERCLIENT, Msg_Dump_Server },			/* Ok */
	{ RPL_LUSEROP, Msg_LUSERS_Various },			/*	Ok */
	{ RPL_LUSERUNKNOWN, Msg_LUSERS_Various },		/*	Ok */
	{ RPL_LUSERCHANNELS, Msg_LUSERS_Various },	/*	Ok */
	{ RPL_LUSERME, Msg_Dump_Server },				/* Ok */
	{ RPL_AWAY, Msg_AWAY },								/* Ok */
	{ RPL_ISON, Msg_ISON },								/* Seems Ok - but needs more checks */
	{ RPL_UNAWAY, Msg_AWAY_Status },					/* Ok */
	{ RPL_NOWAWAY, Msg_AWAY_Status },				/* Ok */
	{ RPL_WHOISUSER, Msg_WHOISUSER },				/* Ok */
	{ RPL_WHOISSERVER, Msg_WHOISSERVER },			/* Ok */
	{ RPL_WHOISOPERATOR, Msg_WHOISOPERATOR },		/* Ok */
	{ RPL_WHOWASUSER, Msg_WHOWASUSER },				/* Ok */
	{ RPL_ENDOFWHO, Msg_ENDOFWHO },					/* Ok */
	{ RPL_WHOISIDLE, Msg_WHOISIDLE },				/* Ok */
	{ RPL_ENDOFWHOIS, Msg_ENDOFWHOIS },				/* Ok */
	{ RPL_WHOISCHANNELS, Msg_WHOISCHANNELS },		/* Ok */
	{ RPL_CREATIONTIME, Msg_CREATIONTIME },		/* Ok */
	{ RPL_NOTOPIC, Msg_Topic },						/* Ok */
	{ RPL_TOPIC, Msg_Topic },							/* Seems Ok (check more closely) */
	{ RPL_TOPICWHOTIME, Msg_TOPICWHOTIME },		/*	Ok */
	{ RPL_LIST, Msg_LIST },								/* To do */
	{ RPL_LISTEND, Msg_LISTEND },						/* Ok */
	{ RPL_CHANNELMODEIS, Msg_Mode },					/* Ok */
	{ RPL_INVITELIST, Msg_INVITELIST },				/* Ok */
	{ RPL_EXCEPTLIST, Msg_EXCEPTLIST },				/* Ok */
	{ RPL_ENDOFINVITELIST, Msg_ENDOFINVITELIST },/* Ok */
	{ RPL_ENDOFEXCEPTLIST, Msg_ENDOFEXCEPTLIST },/* Ok */
	{ RPL_INVITING, Msg_INVITING },					/* To do */
	{ RPL_WHOREPLY, Msg_WHOREPLY },					/* To do */
	{ RPL_NAMREPLY, Msg_NAMREPLY },					/* Server sends unexpected format ! */
	{ RPL_LINKS, Msg_LINKS },							/* Works but needs to be rewritten */
	{ RPL_ENDOFLINKS, Msg_ENDOFLINKS },				/* Ok */
	{ RPL_ENDOFNAMES, Msg_ENDOFNAMES },				/* Ok */
	{ RPL_BANLIST, Msg_BANLIST },						/* To do */
	{ RPL_ENDOFBANLIST, Msg_ENDOFBANLIST },		/* Ok */
	{ RPL_ENDOFWHOWAS, Msg_ENDOFWHOWAS },			/* Ok */
	{ RPL_MOTD, Msg_MOTD },								/* Ok */
	{ RPL_INFO, Msg_INFO },	
	{ RPL_INFOSTART, Msg_INFOSTART },				/* Ok */
	{ RPL_ENDOFINFO, Msg_ENDOFINFO },				/* Ok */
	{ RPL_MOTDSTART, Msg_MOTDSTART },				/* Ok */
	{ RPL_ENDOFMOTD, Msg_ENDOFMOTD },				/* Ok */

	/* Error numerics */

	{ ERR_BANNEDFROMCHAN, Msg_ERR_BANNEDFROMCHAN },
	{ ERR_INVITEONLYCHAN, Msg_ERR_INVITEONLYCHAN },
	{ ERR_CHANNELISFULL, Msg_ERR_CHANNELISFULL },
	{ ERR_BADCHANNELKEY, Msg_ERR_BADCHANNELKEY },
	{ ERR_TOOMANYCHANNELS, Msg_ERR_TOOMANYCHANNELS },
	{ ERR_UNAVAILRESOURCE, Msg_ERR_UNAVAILRESOURCE },

	/* End of list */

	{ 0, NULL }
};

/* ----- Message parser -------------------------------------------------------------------------------- */

void Message_Parse(Server *s, gchar *str)
{
	gchar *seek, *end;
	struct Message *Msg;
	gint i;
	gboolean parsed = FALSE;

	g_return_if_fail(s);
	g_return_if_fail(str);

	if (s->Raw_Window) Server_Raw_Window_Output(s, str, FALSE);

	if (!*str) return;	/* If the message is empty, ignore it */

	seek = strcpy(pmsg_tmp, str);

	Msg = g_malloc0(sizeof(struct Message));	/* We create the Message structure */

	Msg->server = s;

	end = pmsg_tmp + strlen(pmsg_tmp);

	if (end - pmsg_tmp > 512)
	{
		g_warning("Message_Parse(): Got an IRC Message exceeding 512 bytes ?! Dropped.");
		g_free(Msg);
		return;
	}

	/* We remove EOL chars */
	while (end > pmsg_tmp && (*(end-1) == '\n' || *(end-1) == '\r')) *--end = 0;

	if (end <= pmsg_tmp)		/* If the message is empty, ignore it */
	{
		g_free(Msg);
		return;
	}

	if (*pmsg_tmp == ':')	/* Message prefix (optionnal) */
	{
		Msg->prefix = ++seek;
		while (seek<end && *seek!=' ') seek++;
		if (seek<end) *seek++ = 0;
	}

	while (seek<end && *seek==' ') seek++;		/* Skip spaces before the command */

	if (seek<end)				/* Message Command */
	{
		Msg->command = seek;
		Msg->c_num = atoi(seek);
		while (seek<end && *seek!=' ') seek++;
		if (seek<end) *seek++ = 0;
		while (seek<end && *seek==' ') seek++;
	}
	else
	{
		g_warning("Message_Parse(): Got an IRC Message without command !? Dropped.");
		g_free(Msg);
		return;
	}

	while (seek<end)		/* Find each Parameter of the Message Command */
	{
		Msg->args[Msg->nargs++] = seek;
		if (*seek==':') break;
		while (seek<end && *seek!=' ') seek++;
		if (seek<end && *seek==' ') *seek++ = 0;
		while (seek<end && *seek==' ') seek++;
	}

	if (!Msg->command) { g_free(Msg); return; }

	if (Msg->prefix)
	{
		Msg->nick = g_strdup(nick_part(Msg->prefix));
		Msg->userhost = g_strdup(userhost_part(Msg->prefix));
	}

	/* Try to find the command in one of the tables */

	i = 0;

	if (Msg->c_num) while (IRC_Numerics[i].Numeric)	/* Numeric message */
	{
		if (Msg->c_num == IRC_Numerics[i].Numeric)
		{
			IRC_Numerics[i].Function(Msg);
			parsed = TRUE;
			break;
		}
		i++;
	}
	else while (*IRC_Messages[i].Command)	/* Command message */
	{
		if (!g_strcasecmp(Msg->command,IRC_Messages[i].Command))
		{
			IRC_Messages[i].Function(Msg);
			parsed = TRUE;
			break;
		}
		i++;
	}

	if (!parsed)
	{
		if (Msg->c_num>=400 && Msg->c_num<=499)	/* Error message */
		{
			sprintf(pmsg_msg,"Error %d:", Msg->c_num);
			for (i=1; i<Msg->nargs; i++) strspacecat(pmsg_msg,Msg->args[i]);
			VW_output(Msg->server->vw_active, T_WARNING, "sF-", Msg->server, "%s", pmsg_msg);

			if (!s->current_nick)
			{
				gchar *reason = NULL;

				switch (Msg->c_num)
				{
					case ERR_NONICKNAMEGIVEN:
					case ERR_ERRONEUSNICKNAME: reason = "invalid"; break;
					case ERR_NICKNAMEINUSE: reason = "already in use"; break;
					case ERR_NICKCOLLISION: reason = "already registred"; break;
					case ERR_UNAVAILRESOURCE: reason = "temporarily unavailable"; break;
				}

				if (reason)
				{
					sprintf(pmsg_msg,"This nickname is %s.\nPlease choose another nickname :", reason);
					dialog_server_nick(s, pmsg_msg, s->fs->Nick);
				}
			}
		}
		else if (Msg->c_num)	/* Other numeric message */
		{
			sprintf(pmsg_msg,"(%.3d) ", Msg->c_num);
			for (i=1; i<Msg->nargs; i++) strspacecat(pmsg_msg, Msg->args[i]);
			VW_output(Msg->server->vw_active, T_ANSWER, "sF-", Msg->server, "%s", pmsg_msg);
		}
		else /* Unknown command message */
		{
			sprintf(pmsg_msg,"Unknown server message: %s", Msg->command);
			for (i=0; i<Msg->nargs; i++) strspacecat(pmsg_msg, Msg->args[i]);
			VW_output(Msg->server->vw_active, T_ERROR, "sF-", Msg->server, "%s", pmsg_msg);
		}
	}

	g_free(Msg->nick); g_free(Msg->userhost);	g_free(Msg);
}

/* ----------------------------------------------------------------------------------------------------- */

/* vi: set ts=3: */

