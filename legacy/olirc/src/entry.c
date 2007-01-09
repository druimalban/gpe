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
#include "entry.h"
#include "servers.h"
#include "channels.h"
#include "queries.h"
#include "misc.h"
#include "network.h"
#include "ctcp.h"
#include "servers_dialogs.h"
#include "dialogs.h"
#include "histories.h"
#include "dcc.h"
#include "dns.h"
#include "numerics.h"

#include <gtk/gtk.h>
#include <string.h>

#ifdef USE_TCL
#include "tcl-olirc.h"
#endif

gchar ent_tmp_1[2048];
gchar ent_tmp_2[2048];

struct Command
{
	Virtual_Window *vw;
	gchar *command;
	gchar *help;
	gchar *args;
	gchar *token;
	Server *server;
	Channel *channel;
	Query *query;
};

#define CMD_HANDLER(f) extern gint f(struct Command *cmd)

/* ------------------------------------------------------------------------------------ */

gchar *Find_Channel_Name(struct Command *cmd)
{
	if (cmd->token)
	{
		if (IsChannelName(cmd->token))
		{
			gchar *name;
			tok_next(&name, &(cmd->token));
			return name;
		}
	}

	if (cmd->channel) return cmd->channel->Name;

	return NULL;
}

/* ----- User commands handlers -------------------------------------------------------- */

CMD_HANDLER(Cmd_Join)
{
	gchar *g, *k, *r, *x = NULL;

	if (!tokenize(cmd->args)) return 1;
	if (tok_next(&g, &k)) tok_next(&k, &r);

	while (g)
	{
		r = strchr(g, ','); if (r) *r++ = 0;
		if (*g)
		{
			if (k) { x = strchr(k, ','); if (x) *x++ = 0; }

			if ((*g>='a' && *g<='z') || (*g>='A' && *g <='Z') || (*g>='0' && *g<='9'))
				sprintf(ent_tmp_2, "#%s", g);
			else strcpy(ent_tmp_2, g);
			Channel_Join(Channel_New(cmd->server, ent_tmp_2), (k && *k)? k : NULL);
			k = x;
		}
		g = r;
	}
	return 0;
}

CMD_HANDLER(Cmd_Quit)
{
	olirc_quit(cmd->token, (gboolean) !(cmd->token));
	return 0;
}

CMD_HANDLER(Cmd_Msg)	/* /MSG /PRIVMSG */
{
	if (cmd->token)
	{
		gchar *g, *r;
		Query *q;

		if (tok_next(&g, &r))
		{
			sprintf(ent_tmp_2, "PRIVMSG %s :%s", g, r);
			if (!Server_Output(cmd->server, ent_tmp_2, TRUE)) return 0;
			VW_output(cmd->vw, T_OWN_SEND_MSG, "snudt", cmd->server, cmd->server->current_nick, "userhost", g, r);
			if (!IsChannelName(g) && (q = Query_Find(cmd->server, g)))
				VW_output(q->vw, T_OWN_MSG, "#snudt", q->server, q->server->current_nick, "userhost", q->member.nick, r);
			return 0;
		}
	}
	return 1;
}

CMD_HANDLER(Cmd_Query)
{
	if (cmd->server->State!=SERVER_CONNECTED)
	{
		VW_output(cmd->vw, T_WARNING, "sF-", cmd->server, "%s", "Server not connected.");
		return 0;
	}

	if (cmd->token)
	{
		gchar *g, *r;
		Query *q;
		tok_next(&g, &r);
		q = Query_New(cmd->server, g);
		if (r && *(g_strchug(r))) q->vw->Input((gpointer) q, r);
		return 0;
	}
	return 1;
}

CMD_HANDLER(Cmd_Raw)	/* /RAW /QUOTE */
{
	if (!cmd->token) return 1;

	Server_Output(cmd->server, cmd->args, TRUE);
	return 0;
}

CMD_HANDLER(Cmd_Clear)
{
	VW_Clear(cmd->vw);
	return 0;
}

CMD_HANDLER(Cmd_Close)
{
	if (cmd->vw->pmask.w_type == W_SERVER && cmd->server->State != SERVER_IDLE && cmd->server->State != SERVER_DISCONNECTING)
	{
		GList *g = g_list_append(NULL, (gpointer) cmd->server);
		dialog_quit_servers(g, NULL, cmd->vw, NULL, FALSE);
	}
	else VW_Close(cmd->vw);
	return 0;
}

#ifdef USE_CTCP
CMD_HANDLER(Cmd_Ctcp)
{
	if (cmd->token)
	{
		gchar *g, *r;
		if (tok_next(&g, &r))
		{
			if (!CTCP_Send(cmd->server, g, FALSE, r, NULL, FALSE)) return 0;
			sprintf(ent_tmp_2, "CTCP command sent to %s", g);
			VW_output(cmd->vw, T_CTCP_SEND, "sF-", cmd->server, "%s", ent_tmp_2);
			return 0;
		}
	}
	return 1;
}
#endif

CMD_HANDLER(Cmd_Me)	/* /ME /ACTION */
{
	if ((cmd->vw->pmask.w_type != W_DCC) && (!cmd->channel) && (!cmd->query))
	{
		sprintf(ent_tmp_2, "You can use /%s only in a channel, a query or a dcc chat.", cmd->command);
		VW_output(cmd->vw, T_WARNING, "sF-", cmd->server, "%s", ent_tmp_2);
		return 0;
	}
	else if (!cmd->args) return 1;

#ifdef USE_DCC
	if (cmd->vw->pmask.w_type == W_DCC)
	{
		/* TODO Move this into dcc.c !! */
		DCC *d;
		g_return_val_if_fail(cmd->vw->Input, 0);
		d = (DCC *) cmd->vw->Resource;
		sprintf(ent_tmp_2, "\001ACTION %s\001", cmd->args);
		cmd->vw->Input(cmd->vw->Resource, ent_tmp_2);
		VW_output(cmd->vw, T_OWN_DCC_ACTION, "snudt", d->server, d->my_nick, "userhost", d->nick, cmd->args);
		return 0;
	}
#endif
#ifdef USE_CTCP
	else if (cmd->channel)
	{
		if (!CTCP_Send(cmd->server, cmd->channel->Name, FALSE, "ACTION", cmd->args, FALSE)) return 0;
		VW_output(cmd->vw, T_OWN_CHAN_ACTION, "snudt", cmd->server, cmd->server->current_nick, "userhost", cmd->channel->Name, cmd->args);
	}
	else if (cmd->vw->pmask.w_type == W_QUERY)
	{
		if (!CTCP_Send(cmd->server, cmd->query->member.nick, FALSE, "ACTION", cmd->args, FALSE)) return 0;
		VW_output(cmd->vw, T_OWN_CHAN_ACTION, "snudt", cmd->server, cmd->server->current_nick, "userhost", cmd->query->member.nick, cmd->args);
	}
#endif

	return 0;
}

CMD_HANDLER(Cmd_Topic)
{
	gchar *chan;
	if (!(chan = Find_Channel_Name(cmd))) return 4;
	if (!cmd->token) sprintf(ent_tmp_2, "TOPIC %s", chan);
	else sprintf(ent_tmp_2, "TOPIC %s :%s", chan, cmd->token);
	Server_Output(cmd->server, ent_tmp_2, TRUE);
	return 0;
}

CMD_HANDLER(Cmd_Names)
{
	gchar *chan;
	chan = Find_Channel_Name(cmd);
	if (chan) sprintf(ent_tmp_2, "NAMES %s", chan);
	else strcpy(ent_tmp_2, "NAMES");
	Server_Output(cmd->server, ent_tmp_2, TRUE);
	return 0;
}

CMD_HANDLER(Cmd_Banlist)
{
	gchar *chan;
	if (!(chan = Find_Channel_Name(cmd))) return 4;
	sprintf(ent_tmp_2, "MODE %s -b", chan);
	Server_Output(cmd->server, ent_tmp_2, TRUE);
	return 0;
}

CMD_HANDLER(Cmd_Simple)	/* /NICK /WHOIS /WHOWAS */
{
	if (!cmd->token && !g_strcasecmp(cmd->command, "NICK"))
		dialog_server_nick(cmd->server, "Please enter a new nickname :", cmd->server->current_nick);
	else if (!cmd->token)
	{
		sprintf(ent_tmp_2, "Usage: /%s nickname", cmd->command);
		VW_output(cmd->vw, T_WARNING, "sF-", cmd->server, "%s", ent_tmp_2);
	}
	else
	{
		sprintf(ent_tmp_2, "%s %s", cmd->command, cmd->token);
		Server_Output(cmd->server, ent_tmp_2, TRUE);
	}
	return 0;
}

CMD_HANDLER(Cmd_Who)
{
	if (!cmd->token) strcpy(ent_tmp_2, "WHO");
	else sprintf(ent_tmp_2, "WHO %s", cmd->token);
	Server_Output(cmd->server, ent_tmp_2, TRUE);
	return 0;
}

CMD_HANDLER(Cmd_Mode)
{
	gchar *g, *r;

	tok_next(&g, &r); if (!g) return 1;

	strcpy(ent_tmp_2, "MODE ");

	for(;;)
	{
		strspacecat(ent_tmp_2, g);
		tok_next(&g, &r); if (!g) break;
	}
	Server_Output(cmd->server, ent_tmp_2, TRUE);
	return 0;
}

CMD_HANDLER(Cmd_Op_Voice) /* /OP /VOICE /DEOP /DEVOICE */
{
	gchar *chan;
	gchar *g, *r, *a;

	chan = Find_Channel_Name(cmd);
	if (!cmd->token) return 1;
	if (!chan) return 4;
		
	if (!g_strcasecmp(cmd->command, "OP")) a="+o";
	else if (!g_strcasecmp(cmd->command, "VOICE")) a="+v";
	else if (!g_strcasecmp(cmd->command, "DEOP")) a="-o";
	else if (!g_strcasecmp(cmd->command, "DEVOICE")) a="-v";
	else return 0;
	sprintf(ent_tmp_2, "MODE %s", chan);
	for(;;)
	{
		tok_next(&g, &r); if (!g) break;
		strspacecat(ent_tmp_2, a);
		strspacecat(ent_tmp_2, g);
	}
	Server_Output(cmd->server, ent_tmp_2, TRUE);
	return 0;
}

CMD_HANDLER(Cmd_Part)
{
	gchar *chan;
	Channel *c;
	if (!(chan = Find_Channel_Name(cmd))) return 4;
	if (!(c = Channel_Find(cmd->server, chan))) return 0;
	Channel_Part(c, (cmd->token)? cmd->token : NULL);
	return 0;
}

CMD_HANDLER(Cmd_Time)	/* /DATE /TIME */
{
	Server_Output(cmd->server, "TIME", TRUE);
	return 0;
}

CMD_HANDLER(Cmd_Notice)
{
	gchar *g, *r;
	if (!tok_next(&g, &r)) return 1;
	sprintf(ent_tmp_2, "NOTICE %s :%s", g, r);
	if (!Server_Output(cmd->server, ent_tmp_2, TRUE)) return 0;
	VW_output(cmd->vw, T_OWN_SEND_NOTICE, "snudt", cmd->server, cmd->server->current_nick, "userhost", g, r);
	return 0;
}

CMD_HANDLER(Cmd_Invite)
{
	gchar *g, *r;
	gchar *n;
	tok_next(&g, &r);
	if (!g || (!r && !cmd->channel)) return 1;
	n = g;
	if (!r) r = cmd->channel->Name;
	else tok_next(&r, &g);
	sprintf(ent_tmp_2, "INVITE %s %s", n, r);
	Server_Output(cmd->server, ent_tmp_2, TRUE);
	return 0;
}

#ifdef USE_CTCP
CMD_HANDLER(Cmd_Simple_Ctcp) /* /CLIENTINFO /FINGER /PING /USERINFO /VERSION */
{
	gchar *g, *r;

	if (!cmd->token) return 1;

	do
	{
		tok_next(&g, &r);
		if (!g_strcasecmp(cmd->command, "PING"))
		{
			if (!CTCP_Send(cmd->server, g, FALSE, "PING", Create_CTCP_PING_Data(), TRUE))
			return 0;
		}
		else if (!CTCP_Send(cmd->server, g, FALSE, cmd->command, NULL, TRUE)) return 0;
	} while (r);

	return 0;
}
#endif

CMD_HANDLER(Cmd_Ison)
{
	if (!cmd->token) return 1;
	sprintf(ent_tmp_2, "ISON %s", cmd->args);
	Server_Output(cmd->server, ent_tmp_2, TRUE);
	return 0;
}

CMD_HANDLER(Cmd_Away)
{
	if (!cmd->token) strcpy(ent_tmp_2, "AWAY");
	else sprintf(ent_tmp_2, "AWAY :%s", cmd->args);
	Server_Output(cmd->server, ent_tmp_2, TRUE);
	return 0;
}

CMD_HANDLER(Cmd_Links)	/* /LINKS /LUSERS */
{
	Server_Output(cmd->server, cmd->command, TRUE);
	return 0;
}

CMD_HANDLER(Cmd_Dcc)
{
	gchar *g, *r, *seek;

	if (!cmd->token) return 1;

	tok_next(&g, &r);

	if (!g_strcasecmp(g, "LIST"))
	{
		/* DCC_Display_Connections(); */
		return 2;
	}

	if (g_strcasecmp(g, "CHAT") && g_strcasecmp(g, "SEND")) return 1;

	if (!cmd->server)
	{
		VW_output(cmd->vw, T_WARNING, "sF-", NULL, "%s", "No server attached to this window.");
		return 0;
	}
	else if (cmd->server->State != SERVER_CONNECTED)
	{
		sprintf(ent_tmp_2, "Server %s is not connected.", cmd->server->fs->Name);
		VW_output(cmd->vw, T_WARNING, "sF-", cmd->server, "%s", ent_tmp_2);
		return 0;
	}
	else if (!Our_IP)
	{
		VW_output(cmd->vw, T_WARNING, "sF-", cmd->server, "%s", "Unable to guess the local IP address.");
		return 0;
	}

#ifdef USE_DCC
	if (!g_strcasecmp(g, "SEND"))
	{
		gchar *n;
		if (!r) return 1;
		tok_next(&g ,&r); if (!g) return 1;
		n = g;
		if ((seek = strchr(g, ':'))) return 2;
		tok_next(&g ,&r);
		if (g) DCC_Send_New(n, cmd->server, g);
		else DCC_Send_Filesel(cmd->server, n);
	}
	else
	{
		tok_next(&g, &r); if (!g) return 1;
		if ((seek = strchr(g, ':'))) return 2;
		DCC_Chat_New(g, cmd->server, 0, 0);
	}
#endif

	return 0;
}

CMD_HANDLER(Cmd_Kick)
{
	gchar *g, *r;
	gchar *chan;

	if (!(chan = Find_Channel_Name(cmd))) return 4;
	if (!cmd->token) return 1;
	tok_next(&g, &r);

	if (r) sprintf(ent_tmp_2, "KICK %s %s :%s", chan, g, r);
	else sprintf(ent_tmp_2, "KICK %s %s :%s", chan, g, cmd->server->current_nick);
	Server_Output(cmd->server, ent_tmp_2, TRUE);

	return 0;
}

CMD_HANDLER(Cmd_Ban)	/* /BAN /UBAN */
{
	gchar tmp[512];
	gchar *g, *r;
	gchar *chan;
	gint i = 0, j;

	if (!(chan = Find_Channel_Name(cmd))) return 4;
	if (!cmd->token) return 1;

	*tmp = 0;

	for(;;)
	{
		tok_next(&g, &r); if (!g) break;
		strspacecat(tmp, g);
		i++;
	}

	sprintf(ent_tmp_2, "MODE %s %c", chan, (!g_strcasecmp(cmd->command, "UBAN"))? '-' : '+');
	for (j = 0; j<i; j++) strcat(ent_tmp_2, "b");
	strspacecat(ent_tmp_2, tmp);

	Server_Output(cmd->server, ent_tmp_2, TRUE);

	return 0;
}

CMD_HANDLER(Cmd_List)
{
	if (!cmd->token) strcpy(ent_tmp_2, "LIST");
	else sprintf(ent_tmp_2, "LIST %s", cmd->args);
	Server_Output(cmd->server, ent_tmp_2, TRUE);
	return 0;
}

CMD_HANDLER(Cmd_Userhost)
{
	if (!cmd->token) return 1;
	sprintf(ent_tmp_2, "USERHOST %s", cmd->args);
	Server_Output(cmd->server, ent_tmp_2, TRUE);
	return 0;
}

CMD_HANDLER(Cmd_Motd)
{
	if (!cmd->token) strcpy(ent_tmp_2, "MOTD");
	else sprintf(ent_tmp_2, "MOTD %s", cmd->args);
	Server_Output(cmd->server, ent_tmp_2, TRUE);
	return 0;
}

CMD_HANDLER(Cmd_Trace)
{
	if (!cmd->token) strcpy(ent_tmp_2, "TRACE");
	else sprintf(ent_tmp_2, "TRACE %s", cmd->args);
	Server_Output(cmd->server, ent_tmp_2, TRUE);
	return 0;
}

/* ----- /DNS Command ----- */

/* FIXME If a server is closed while waiting for the a WHO reply,
	the cmd_dns_request structs allocated at this time are never freed */

struct cmd_dns_request
{
	gchar *requested;
	guint32 rwid;
	guint32 srvid;
	gboolean nick;
	gboolean ip;
};

void cmd_dns_dns_callback(struct olirc_hostip *oh, gpointer data)
{
	Virtual_Window *vw = NULL;
	struct cmd_dns_request *dr = (struct cmd_dns_request *) data;

	if (dr->nick)
	{
		Server *s = Server_find_by_id(dr->srvid);
		g_return_if_fail(s);
		vw = s->vw_active;
	}
	else
	{
		GUI_Window *rw = GW_find_by_id(dr->rwid);
		if (!rw) { g_free(dr->requested); g_free(dr); return; }
		vw = rw->vw_active;
	}

	if (oh->s_error || oh->h_error) sprintf(ent_tmp_2, "Unable to resolve %s (%s)", dr->requested, dns_error(oh));
	else 
	{
		if (!dr->ip) sprintf(ent_tmp_2, "Resolved %s to %s", dr->requested, Dump_IP(oh->ip));
		else sprintf(ent_tmp_2, "Resolved %s to %s", dr->requested, oh->host);
	}
	VW_output(vw, T_WARNING, "F-", "%s", ent_tmp_2);

	g_free(dr->requested); g_free(dr);
}

gboolean cmd_dns_who_callback(struct Message *m, gpointer data)
{
	struct cmd_dns_request *dr = (struct cmd_dns_request *) data;
	Server *s;

	s = Server_find_by_id(dr->srvid);
	g_return_val_if_fail(s, FALSE);

	if (!m)
	{
		sprintf(ent_tmp_2, "%s: No such nick", dr->requested);
		VW_output(s->vw_active, T_WARNING, "sF-", s, "%s", ent_tmp_2);
		g_free(dr->requested); g_free(dr);
		return FALSE;
	}

	sprintf(ent_tmp_2, "%s is %s!%s@%s", dr->requested, m->args[5], m->args[2], m->args[3]);
	VW_output(s->vw_active, T_WARNING, "sF-", s, "%s", ent_tmp_2);

	sprintf(ent_tmp_2, "%s!%s@%s", m->args[5], m->args[2], m->args[3]);
	Channels_Member_Userhost_Store(s, ent_tmp_2);

	sprintf(ent_tmp_2, "Resolving %s...", m->args[3]);
	VW_output(s->vw_active, T_WARNING, "sF-", s, "%s", ent_tmp_2);

	g_free_and_set(dr->requested, g_strdup(m->args[3]));

	if (is_string_an_IP(dr->requested))
	{
		dr->ip = TRUE;
		dns_resolve(convert_IP(dr->requested), NULL, cmd_dns_dns_callback, (gpointer) dr);
	}
	else dns_resolve(0, dr->requested, cmd_dns_dns_callback, (gpointer) dr);

	return FALSE;
}

CMD_HANDLER(Cmd_Dns)
{
	gchar *g, *r;

	if (!cmd->token) return 1;

	for (;;)
	{
		tok_next(&g, &r);
		if (!g) break;
		r = g; while (*r && *r !='.') r++;
		if (*r == '.')
		{
			struct cmd_dns_request *dr = (struct cmd_dns_request *) g_malloc0(sizeof(struct cmd_dns_request));
			if (cmd->server) dr->srvid = cmd->server->object_id;
			dr->rwid = cmd->vw->rw->object_id;

			dr->requested = g_strdup(g);
			sprintf(ent_tmp_2, "Resolving %s...", g);
			VW_output(cmd->vw, T_WARNING, "sF-", cmd->server, "%s", ent_tmp_2);

			if (is_string_an_IP(g))
			{
				dr->ip = TRUE;
				dns_resolve(convert_IP(g), NULL, cmd_dns_dns_callback, (gpointer) dr);
			}
			else dns_resolve(0, g, cmd_dns_dns_callback, (gpointer) dr);
		}
		else if (cmd->vw->pmask.w_type != W_CONSOLE && cmd->server && cmd->server->State == SERVER_CONNECTED)
		{
			struct cmd_dns_request *dr = (struct cmd_dns_request *) g_malloc0(sizeof(struct cmd_dns_request));

			if (cmd->server) dr->srvid = cmd->server->object_id;
			dr->rwid = cmd->vw->rw->object_id;

			dr->requested = g_strdup(g);
			dr->nick = TRUE;

			if (Server_Callback_Add(cmd->server, RPL_WHOREPLY, g, 0, cmd_dns_who_callback, (gpointer) dr))
			{
				sprintf(ent_tmp_2, "Retrieving %s userhost mask...", g);
				VW_output(cmd->vw, T_WARNING, "sF-", cmd->server, "%s", ent_tmp_2);
				sprintf(ent_tmp_2, "WHO %s", g);
				Server_Output(cmd->server, ent_tmp_2, FALSE);
			}
		}
		else
		{
			if (cmd->vw->pmask.w_type == W_CONSOLE) strcpy(ent_tmp_2, "You can't do this in a console.");
			else if (!cmd->server) strcpy(ent_tmp_2, "No IRC server related to this window");
			else strcpy(ent_tmp_2, "Server not connected.");
			VW_output(cmd->vw, T_WARNING, "sF-", cmd->server, "%s", ent_tmp_2);
		}
	}

	return 0;
}

CMD_HANDLER(Cmd_Load) /* /LOAD /SOURCE */
{
	gchar *seek;

	if (!cmd->token) return 1;

	g_strchomp(cmd->token);

	seek = cmd->token + strlen(cmd->token);
	while (seek >= cmd->token && *seek != '.') seek--;

	if (seek < cmd->token) VW_output(cmd->vw, T_WARNING, "t", "Filename must have an extension.");
	else
	{
		#ifdef USE_TCL
			if (!g_strcasecmp(seek, ".tcl"))
			{
				sprintf(ent_tmp_2, "source %s", cmd->token);
				tcl_do(cmd->vw, ent_tmp_2);
				return 0;
			}
		#endif
		#ifdef USE_PYTHON
			if (!g_strcasecmp(seek, ".py"))
			{
				return 0;
			}
		#endif
	
		VW_output(cmd->vw, T_WARNING, "F", "Unknown extension '%s'", seek);
	}

	return 0;
}

/* ----- Not yet implemented ---------------------------------------------------------- */

CMD_HANDLER(Cmd_Server) { return 2; }
CMD_HANDLER(Cmd_Kickban) { return 2; }
CMD_HANDLER(Cmd_Oper) { return 2; }
CMD_HANDLER(Cmd_Stats) { return 2; }
CMD_HANDLER(Cmd_Connect) { return 2; }
CMD_HANDLER(Cmd_Admin) { return 2; }
CMD_HANDLER(Cmd_Info) { return 2; }
CMD_HANDLER(Cmd_Kill) { return 2; }
CMD_HANDLER(Cmd_Die) { return 2; }
CMD_HANDLER(Cmd_Rehash) { return 2; }
CMD_HANDLER(Cmd_Restart) { return 2; }
CMD_HANDLER(Cmd_Wallops) { return 2; }
CMD_HANDLER(Cmd_Ignore) { return 2; }
CMD_HANDLER(Cmd_Exec) { return 2; }
CMD_HANDLER(Cmd_Chat) { return 2; }
CMD_HANDLER(Cmd_Send) { return 2; }

/*******************************************************************************************/

struct { char *Cmd; gint Flags; gint (* Function)(struct Command *); char *Help; } User_Cmds[] =
{
	{ "ACTION", W_WITH_SERVER, Cmd_Me, "message" },
	{ "ADMIN", W_WITH_SERVER, Cmd_Admin, NULL },
	{ "AWAY", W_WITH_SERVER, Cmd_Away, "[reason]" },
	{ "BAN", W_WITH_SERVER, Cmd_Ban, "mask [mask ...]" },
	{ "BANLIST", W_WITH_SERVER, Cmd_Banlist, "[#channel]" },
	{ "CHAT", W_WITH_SERVER, Cmd_Chat, "nickname" },
	{ "CLEAR", W_CONSOLE | W_DCC | W_WITH_SERVER, Cmd_Clear, NULL },
#ifdef USE_CTCP
	{ "CLIENTINFO", W_WITH_SERVER, Cmd_Simple_Ctcp, "{nick | #channel}" },
#endif
	{ "CLOSE", W_CONSOLE | W_DCC | W_WITH_SERVER, Cmd_Close, NULL },
	{ "CONNECT", W_WITH_SERVER, Cmd_Connect, NULL },
#ifdef USE_CTCP
	{ "CTCP", W_WITH_SERVER, Cmd_Ctcp, "nick command [parameters]" },
#endif
	{ "DATE", W_WITH_SERVER, Cmd_Time, NULL },
#ifdef USE_DCC
	{ "DCC", W_WITH_SERVER | W_DCC | W_CONSOLE, Cmd_Dcc, "{CHAT | SEND | LIST} [nick] [file]" },
#endif
	{ "DEOP", W_WITH_SERVER, Cmd_Op_Voice, "[#channel] nick [nick ...]" },
	{ "DEVOICE", W_WITH_SERVER, Cmd_Op_Voice, "[#channel] nick [nick ...]" },
	{ "DIE", W_WITH_SERVER, Cmd_Die, NULL },
	{ "DNS", W_CONSOLE | W_DCC | W_WITH_SERVER, Cmd_Dns, "nick | hostmask | IP" },
	{ "EXEC", W_WITH_SERVER, Cmd_Exec, NULL },
#ifdef USE_CTCP
	{ "FINGER", W_WITH_SERVER, Cmd_Simple_Ctcp, "{nick | #channel}" },
#endif
	{ "IGNORE", W_WITH_SERVER, Cmd_Ignore, NULL },
	{ "INFO", W_WITH_SERVER, Cmd_Info, NULL },
	{ "INVITE", W_WITH_SERVER, Cmd_Invite, "nick [#channel]" },
	{ "ISON", W_WITH_SERVER, Cmd_Ison, "nick [nick ...]" },
	{ "JOIN", W_WITH_SERVER, Cmd_Join, "#channel [,#channel,...] [key [,key,...]]" },
	{ "KICK", W_WITH_SERVER, Cmd_Kick, "nick [reason]" },
	{ "KICKBAN", W_WITH_SERVER, Cmd_Kickban, "nick [reason]" },
	{ "KILL", W_WITH_SERVER, Cmd_Kill, NULL },
	{ "LINKS", W_WITH_SERVER, Cmd_Links, NULL },
	{ "LIST", W_WITH_SERVER, Cmd_List, "[mask] [-min n] [-max n]" },
	{ "LOAD", W_CONSOLE | W_DCC | W_WITH_SERVER, Cmd_Load, "filename" },
	{ "LUSERS", W_WITH_SERVER, Cmd_Links, NULL },
	{ "ME", W_WITH_SERVER | W_DCC, Cmd_Me, "message" },
	{ "MODE", W_WITH_SERVER, Cmd_Mode, "{nick | #channel} [parameters]" },
	{ "MOTD", W_WITH_SERVER, Cmd_Motd, "[server]" },
	{ "MSG", W_WITH_SERVER, Cmd_Msg, "{nick | #channel} message" },
	{ "NAMES", W_WITH_SERVER, Cmd_Names, "[#channel]" },
	{ "NICK", W_WITH_SERVER, Cmd_Simple, "new_nick" },
	{ "NOTICE", W_WITH_SERVER, Cmd_Notice, "{nick | #channel} message" },
	{ "OP", W_WITH_SERVER, Cmd_Op_Voice, "[#channel] nick [nick ...]" },
	{ "OPER", W_WITH_SERVER, Cmd_Oper, NULL },
	{ "PART", W_WITH_SERVER, Cmd_Part, "[#channel]" },
#ifdef USE_CTCP
	{ "PING", W_WITH_SERVER, Cmd_Simple_Ctcp, "{nick | #channel}" },
#endif
	{ "PRIVMSG", W_WITH_SERVER, Cmd_Msg, "{nick | #channel} message" },
	{ "QUERY", W_WITH_SERVER, Cmd_Query, "nick [message]" },
	{ "QUIT", W_CONSOLE | W_DCC | W_WITH_SERVER, Cmd_Quit, "[message]" },
	{ "QUOTE", W_WITH_SERVER, Cmd_Raw, "parameters" },
	{ "RAW", W_WITH_SERVER, Cmd_Raw, "parameters" },
	{ "REHASH", W_WITH_SERVER, Cmd_Rehash, NULL },
	{ "RESTART", W_WITH_SERVER, Cmd_Restart, NULL },
	{ "SEND", W_WITH_SERVER, Cmd_Send, "nickname [filename]" },
	{ "SERVER", W_WITH_SERVER, Cmd_Server, "server [port]" },
	{ "SOURCE", W_CONSOLE | W_DCC | W_WITH_SERVER, Cmd_Load, "filename" },
	{ "STATS", W_WITH_SERVER, Cmd_Stats , NULL},
	{ "TIME", W_WITH_SERVER, Cmd_Time, NULL },
	{ "TOPIC", W_WITH_SERVER, Cmd_Topic, "[#channel] [new_topic]" },
	{ "TRACE", W_WITH_SERVER, Cmd_Trace, "{nick | server }" },
	{ "UBAN", W_WITH_SERVER, Cmd_Ban, "mask [mask ...]" },
	{ "USERHOST", W_WITH_SERVER, Cmd_Userhost, "nick [nick...]" },
#ifdef USE_CTCP
	{ "USERINFO", W_WITH_SERVER, Cmd_Simple_Ctcp, "{nick | #channel}" },
#endif
	/* <PREF> /version must do a CTCP VERSION | a SERVER VERSION */
#ifdef USE_CTCP
	{ "VERSION", W_WITH_SERVER, Cmd_Simple_Ctcp, "[nick | #channel]" },
#endif
	{ "VOICE", W_WITH_SERVER, Cmd_Op_Voice, "[#channel] nick [nick ...]" },
	{ "WALLOPS", W_WITH_SERVER, Cmd_Wallops, NULL },
	{ "WHO", W_WITH_SERVER, Cmd_Who, "[mask]" },
	{ "WHOIS", W_WITH_SERVER, Cmd_Simple, "nick" },
	{ "WHOWAS", W_WITH_SERVER, Cmd_Simple, "nick" },
	{ NULL }
};

/*******************************************************************************************/

GList *Parser_Commands = NULL;

struct Parser_Command
{
	gchar *cmd;
	gint flags;
	gint handler_type;
	gpointer handler;
	gint min_params;
	gint max_params;				/* -1 = not limited */
	gchar *help_string;
};

struct Parsed_Command
{
	struct Parser_Command *command;
	Virtual_Window *vw;			 /* Could be NULL (if called from a script) */
	gchar *args;
	gchar *token;
	Server *server;
	Channel *channel;
	Query *query;
};

gint parser_command_cmp(const gpointer a, const gpointer b)
{
	return g_strcasecmp(((struct Parser_Command *) a)->cmd, ((struct Parser_Command *) b)->cmd);
}

void parser_add_command(gchar *cmd, gint flags, gint h_type, gpointer handler, gint min_p, gint max_p, gchar *help)
{
	struct Parser_Command *pc;

	g_return_if_fail(cmd);
	g_return_if_fail(handler);

	pc = g_malloc0(sizeof(struct Parser_Command));

	pc->cmd = g_strdup(cmd); // g_strup(pc->cmd);
	pc->flags = flags;
	pc->handler_type = h_type;
	if (h_type == HANDLER_BUILTIN) pc->handler = handler;
	else pc->handler = g_strdup(handler);
	pc->min_params = min_p;
	pc->max_params = max_p;
	pc->help_string = g_strdup(help);

	Parser_Commands = g_list_insert_sorted(Parser_Commands, pc, (GCompareFunc) parser_command_cmp);
}

void parser_remove_command(gchar *cmd, gint h_type)
{
	GList *l = Parser_Commands;

	while (l)
	{
		if (((struct Parser_Command *) l->data)->handler_type == h_type && g_strcasecmp(cmd, ((struct Parser_Command *) l->data)->cmd) == 0)
		{
			g_free(((struct Parser_Command *) l->data)->cmd);
			g_free(((struct Parser_Command *) l->data)->help_string);
			if (((struct Parser_Command *) l->data)->handler_type == HANDLER_BUILTIN) g_free(((struct Parser_Command *) l->data)->handler);
			Parser_Commands = g_list_remove(Parser_Commands, l->data);
			return;
		}
		l = l->next;
	}
}

void init_entry_parser_commands()
{
	gint i = 0;

	while (User_Cmds[i].Cmd)
	{
		parser_add_command(User_Cmds[i].Cmd, User_Cmds[i].Flags, HANDLER_BUILTIN, (gpointer) User_Cmds[i].Function, 0, -1, User_Cmds[i].Help);
		i++;
	}
}

/* ----- Tab completion --------------------------------------------------------------- */

void Entry_Tab(Virtual_Window *vw, guint pos, gboolean desc)
{
	static gchar comp[256];
	gchar *seek;

	if (vw->Cmd_Index)
	{
		if (desc) vw->Cmd_Index = vw->Cmd_Index->prev;
		else vw->Cmd_Index = vw->Cmd_Index->next;

		strcpy(comp, vw->Completing); g_strup(comp);

		if (vw->Cmd_Index == NULL || g_strncasecmp(((struct Parser_Command *) vw->Cmd_Index->data)->cmd, comp, strlen(comp)))
		{
			vw->Cmd_Index = NULL;
			sprintf(ent_tmp_1, "%s/%s%s", vw->Before, vw->Completing, vw->After);
			gtk_entry_set_text((GtkEntry *) vw->entry, ent_tmp_1);
			gtk_entry_set_position((GtkEntry *) vw->entry, strlen(vw->Before)+strlen(vw->Completing)+1);
			g_free(vw->Before); g_free(vw->Completing); g_free(vw->After);
		}
		else
		{
			strcpy(ent_tmp_2, ((struct Parser_Command *) vw->Cmd_Index->data)->cmd);
			g_strdown(ent_tmp_2);
			sprintf(ent_tmp_1, "%s/%s %s", vw->Before, ent_tmp_2, vw->After);
			gtk_entry_set_text((GtkEntry *) vw->entry, ent_tmp_1);
			gtk_entry_set_position((GtkEntry *) vw->entry, strlen(vw->Before)+strlen(ent_tmp_2)+2);
		}
		return;
	}
	else if (vw->Member_Index)
	{
		GList *l = vw->Member_Index;

		if (desc) l = l->prev;
		else l = l->next;

		if (!l || irc_ncmp(((Member *) l->data)->nick, comp, strlen(comp)))
		{
			sprintf(ent_tmp_1, "%s%s%s", vw->Before, vw->Completing, vw->After);
			gtk_entry_set_text((GtkEntry *) vw->entry, ent_tmp_1);
			gtk_entry_set_position((GtkEntry *) vw->entry, strlen(vw->Before)+strlen(vw->Completing));
			g_free(vw->Before); g_free(vw->Completing); g_free(vw->After);
			vw->Member_Index = NULL;
		}
		else
		{
			gchar *m = ((Member *) l->data)->nick;
			sprintf(ent_tmp_1, "%s%s%s", vw->Before, m, vw->After);
			gtk_entry_set_text((GtkEntry *) vw->entry, ent_tmp_1);
			gtk_entry_set_position((GtkEntry *) vw->entry, strlen(vw->Before)+strlen(m));
			vw->Member_Index = l;
		}
	}
	else
	{
		strcpy(ent_tmp_1, gtk_entry_get_text((GtkEntry *) vw->entry));
		seek = ent_tmp_1 + pos;
		strcpy(ent_tmp_2, seek);
		*seek = 0;

		while (seek>ent_tmp_1 && *(seek-1)!=' ') seek--;

		if (*seek == '/')
		{
			GList *m, *l = Parser_Commands;
			gchar *start = seek;

			if (start>ent_tmp_1) start--;
			while (start>ent_tmp_1 && *start==' ') start--;

			if (start!=ent_tmp_1) return;

			*seek = 0; seek++;

			if (strlen(seek)>32) return;

			strcpy(comp, seek); g_strup(comp);

			while(l && g_strncasecmp(((struct Parser_Command *) l->data)->cmd, comp, strlen(comp)) < 0)
				l = l->next;

			if (!l) return;

			if (g_strncasecmp(((struct Parser_Command *) l->data)->cmd, comp, strlen(comp))) return;

			vw->Before = g_strdup(ent_tmp_1);
			vw->Completing = g_strdup(seek);
			vw->After = g_strdup(ent_tmp_2);

			if (desc)
			{
				m = l->next;
				while (m && !g_strncasecmp(((struct Parser_Command *) m->data)->cmd, comp, strlen(comp)))
				{
					l = m;
					m = m->next;
				}
			}

			vw->Cmd_Index = l;

			strcpy(ent_tmp_2, ((struct Parser_Command *) l->data)->cmd);
			g_strdown(ent_tmp_2);
			sprintf(ent_tmp_1, "%s/%s %s", vw->Before, ent_tmp_2, vw->After);
			gtk_entry_set_text((GtkEntry *) vw->entry, ent_tmp_1);
			gtk_entry_set_position((GtkEntry *) vw->entry, strlen(vw->Before)+strlen(ent_tmp_2)+2);

			return;
		}
		else if (vw->pmask.w_type == W_CHANNEL)
		{
			Channel *c = (Channel *) vw->Resource;
			GList *l = c->C_Members;
			gchar *m;

			if (strlen(seek)>256) return;

			strcpy(comp, seek); *seek = 0;

			while (l && irc_ncmp(((Member *) l->data)->nick, comp, strlen(comp))<0)
				l = l->next;

			if (desc)
				while (l->next && !irc_ncmp(((Member *)l->next->data)->nick, comp, strlen(comp)))
					l = l->next;

			if (l && !irc_ncmp(((Member *) l->data)->nick, comp, strlen(comp)))
			{
				m = ((Member *) l->data)->nick;

				vw->Before = g_strdup(ent_tmp_1);
				vw->Completing = g_strdup(comp);
				vw->After = g_strdup(ent_tmp_2);
				vw->Member_Index = l;

				sprintf(ent_tmp_1, "%s%s%s", vw->Before, m, vw->After);
				gtk_entry_set_text((GtkEntry *) vw->entry, ent_tmp_1);
				gtk_entry_set_position((GtkEntry *) vw->entry, strlen(vw->Before)+strlen(m));
			}
		}
	}
}

/* ----- Parse user entries ----------------------------------------------------------- */

void Parse_Entry(GtkWidget *widget, gpointer data)
{
	Virtual_Window *vw;
	gchar *seek, *start;
	gchar line[sizeof(ent_tmp_1)];
	gboolean cmd = FALSE;

	g_return_if_fail(data);

	vw = (Virtual_Window *) data;

	seek = strcpy(ent_tmp_1, gtk_entry_get_text((GtkEntry *) vw->entry));
	gtk_entry_set_text((GtkEntry *) vw->entry, "");

	while (*seek && (*seek==' ' || *seek=='\t')) seek++;
	if (!*seek) return;

	History_Add(vw, seek);

	if (*seek == '/') cmd = TRUE;

	start = seek; *line = 0;
	while ((seek = strchr(start, '\\')))
	{
		*seek++ = 0;
		strcat(line, start);
		switch (*seek)
		{
			/* <PREFS> Backslash escaping should not be mandatory */

			case 'n':	*seek = '\n'; break;
			case 'r':	*seek = 0x16; break;
			case 'k':	*seek = 0x03; break;
			case 'o':	*seek = 0x0F; break;
			case 'u':	*seek = 0x1F; break;
			case 'b':	*seek = 0x02; break;
			case 'g':	*seek = 0x07; break;
			case 'a':	*seek = 0x01; break;
			case '\\':	seek++; strcat(line, "\\"); break;
			case 'x': case 'X':
			{
				int i, q = 0, v;
				for (i = 1; i <= 2; i++)
				{
					if (!seek[i]) break;
					if (seek[i] >= '0' && seek[i] <='9') v = seek[i] - '0';
					else if (seek[i] >= 'a' && seek[i] <='f') v = seek[i] - 'a' + 10;
					else if (seek[i] >= 'A' && seek[i] <='F') v = seek[i] - 'A' + 10;
					else break;
					q = (q << 4) | (v & 0x0F);
				}
				if (i < 3 || q > 255) break;
				seek += 2; *seek = q;
				break;
			}
			case '0': case '1': case '2': case '3':
			{
				int i, q = 0;
				for (i = 0; i <= 2; i++)
				{
					if (seek[i] < '0' || seek[i] > '7') break;
					q = (q << 3) | ((seek[i] - '0') & 0x07);
				}
				if (i < 3 || q > 255) break;
				seek += 2; *seek = q;
				break;
			}
		}
		start = seek;
	}
	strcat(line, start);

	if (cmd)
	{
		seek = line;
		while (*seek && *seek!=' ' && *seek!='\t') seek++;
		if (*seek) start = seek+1; else start = NULL;
		*seek = 0;
		Do_Command(vw, line+1, start);
	}
	else
	{
		g_return_if_fail(vw->Input);

		start = line;
		while ((seek = strchr(start, '\n')))
		{
			*seek++ = 0;
			vw->Input(vw->Resource, start);
			start = seek;
		}

		vw->Input(vw->Resource, start);
	}
}

/* ----- Find the command and execute it ---------------------------------------------- */

void Do_Command(Virtual_Window *vw, gchar *str, gchar *args)
{
	GList *l;

	struct Command *cmd;

	cmd = (struct Command *) g_malloc0(sizeof(struct Command));

	cmd->vw = vw;
	cmd->args = args;

	switch (vw->pmask.w_type)
	{
		case W_SERVER:
		{
			cmd->server = (Server *) vw->Resource;
			break;
		}
		case W_CHANNEL:
		{
			cmd->channel = (Channel *) vw->Resource;
			cmd->server = (Server *) cmd->channel->server;
			break;
		}
		case W_QUERY:
		{
			cmd->query = (Query *) vw->Resource;
			cmd->server = (Server *) cmd->query->server;
			break;
		}
#ifdef USE_DCC
		case W_DCC:
		{
			cmd->server = (Server *) ((DCC *) vw->Resource)->server;
			break;
		}
#endif
	}

	cmd->token = tokenize(cmd->args);

	l = Parser_Commands;

	while (l)
	{
		if (!g_strcasecmp(((struct Parser_Command *) l->data)->cmd, str))
		{
			struct Parser_Command *pc = (struct Parser_Command *) l->data;

			cmd->command = pc->cmd;
			cmd->help = pc->help_string;

			if (!((pc->flags) & (cmd->vw->pmask.w_type)))
			{
				VW_output(vw, T_NORMAL, "F-", "%s", "You can't use this command in this window.");
			}
			else
			{
				gint ret = 0;

				switch (pc->handler_type)
				{
					case HANDLER_BUILTIN:

						ret = ((gint (*)(struct Command *)) (pc->handler)) (cmd);
						break;

					case HANDLER_TCL:

					#ifdef USE_TCL
						tcl_do(cmd->vw, (gchar *) pc->handler);
					#endif

						ret = 0; /* FIXME ret should have the true value */
						break;
				}

				switch (ret)
				{
					case 0: break;
					case 1:
					{
						sprintf(ent_tmp_2, "Usage: /%s", cmd->command);
						if (cmd->help) { strcat(ent_tmp_2, " "); strcat(ent_tmp_2, cmd->help); }
						VW_output(cmd->vw, T_WARNING, "F-", "%s", ent_tmp_2);
						break;
					}
					case 2: VW_output(cmd->vw, T_WARNING, "t", "[Not yet implemented]"); break;
					case 4:
					{
						sprintf(ent_tmp_2, "/%s: Please specify a channel name.", cmd->command);
						VW_output(cmd->vw, T_WARNING, "F-", "%s", ent_tmp_2);
					}
				}
			}
			g_free(cmd);
			return;
		}
		l = l->next;
	}

	VW_output(vw, T_NORMAL, "F-", "%s", "Unknown command.");

	g_free(cmd);
}

/* vi: set ts=3: */

