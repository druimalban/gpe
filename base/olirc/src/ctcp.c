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
#include "ctcp.h"
#include "servers.h"
#include "network.h"
#include "windows.h"
#include "channels.h"
#include "misc.h"
#include "queries.h"
#include "dcc.h"
#include "prefs.h"
#include "ignores.h"

#include <string.h>
#include <sys/utsname.h>

gchar ctcp_tmp[2048];

/* ------------------------------------------------------------------------------------ */

gchar *Create_CTCP_PING_Data()
{
	static gchar data[32];
	guint rnd, i, sum = 0;
	time_t t = time(NULL);

	rnd = rand()%100;
	if (rnd < 10) rnd += 10 + rand()%80;
	sprintf(data, "%.2d", (unsigned short) rnd);
	rnd = rand()%256;
	sprintf(data + 2, "%.2X", (unsigned char) rnd);
	for (i=0; i<4; i++) *(((unsigned char *) &t)+i) ^= rnd;
	sprintf(data + 4, "%.8lX", (unsigned long int) t);
	for (i=2; i<12; i++) sum += data[i];
	sprintf(data + 12, "%.2X", (unsigned char) sum);
	for (i=2; i<14; i++) if (data[i]<58) if (rand()%2) data[i] += 24;
	return data;
}

time_t Get_CTCP_PING_Value(gchar *data)
{
	time_t t;
	guint rnd, sum, i, rs = 0;

	for (i=2; i<14; i++) if (data[i]>70) data[i] -= 24;
	sscanf(data + 2, "%2X", &rnd);
	sscanf(data + 4, "%8lX", (unsigned long int *) &t);
	sscanf(data + 12, "%2X", &sum);
	for (i=2; i<12; i++) rs += data[i];
	if (rs%256 != sum) return 0;
	for (i=0; i<4; i++) *(((unsigned char *) &t)+i) ^= rnd;
	return t;
}

gboolean Is_CTCP(gchar *msg)
{
	return (*(msg+1)=='\001' && msg[strlen(msg)-1]=='\001');
}

/* ------------------------------------------------------------------------------------ */

gboolean CTCP_Send(Server *s, gchar *target, gboolean answer, gchar *cmd, gchar *params, gboolean log)
{
	static gchar ctcp_send_buf[1024];
	gchar *type = (answer)? "NOTICE" : "PRIVMSG";

	g_return_val_if_fail(s, FALSE);
	g_return_val_if_fail(target, FALSE);
	g_return_val_if_fail(cmd, FALSE);

	if (params) sprintf(ctcp_send_buf, "%s %s :\001%s %s\001", type, target, cmd, params);
	else sprintf(ctcp_send_buf, "%s %s :\001%s\001", type, target, cmd);

	if (!Server_Output(s, ctcp_send_buf, TRUE)) return FALSE;

	if (log) VW_output(s->vw_active, T_CTCP_SEND, "sF--", s, "CTCP %s request sent to %s", cmd, target);

	return TRUE;
}

/* ------------------------------------------------------------------------------------ */

struct CTCP_Message
{
	Server *s;
	gchar *nick;
	gchar *source;
	gchar *target;
	gchar *cmd;
	gchar *params;
	struct pmask pmask;
};

#define CTCP_HANDLER(f) extern void f(struct CTCP_Message *m)

CTCP_HANDLER(CTCP_DCC_Request);
CTCP_HANDLER(CTCP_Ping_Request);
CTCP_HANDLER(CTCP_Version_Request);
CTCP_HANDLER(CTCP_Finger_Request);
CTCP_HANDLER(CTCP_Userinfo_Request);
CTCP_HANDLER(CTCP_Clientinfo_Request);
CTCP_HANDLER(CTCP_Time_Request);
CTCP_HANDLER(CTCP_Action_Request);
CTCP_HANDLER(CTCP_Source_Request);

CTCP_HANDLER(CTCP_Action_Reply);
CTCP_HANDLER(CTCP_Ping_Reply);
CTCP_HANDLER(CTCP_DCC_Reply);

CTCP_HANDLER(CTCP_Request_Dump);
CTCP_HANDLER(CTCP_Reply_Dump);

struct { char *Cmd; void (*Request_Function) (struct CTCP_Message *); void (*Reply_Function) (struct CTCP_Message *); }
CTCP_Handlers[] =
{
	{ "ACTION", CTCP_Action_Request, CTCP_Action_Reply },
	{ "CLIENTINFO", CTCP_Clientinfo_Request, CTCP_Reply_Dump },
	{ "DCC", CTCP_DCC_Request, CTCP_DCC_Reply },
	{ "ECHO", CTCP_Ping_Request, CTCP_Reply_Dump },
	{ "FINGER", CTCP_Finger_Request, CTCP_Reply_Dump },
	{ "PING", CTCP_Ping_Request, CTCP_Ping_Reply },
	{ "SOURCE", CTCP_Source_Request, CTCP_Reply_Dump },
	{ "TIME", CTCP_Time_Request, CTCP_Reply_Dump },
	{ "USERINFO", CTCP_Userinfo_Request, CTCP_Reply_Dump },
	{ "VERSION", CTCP_Version_Request, CTCP_Reply_Dump },
	{ NULL }
};

void CTCP_Parse(Server *s, gchar *source, gchar *target, gboolean reply, gchar *msg)
{
	static gchar ctcp_parse_buf[1024];
	gchar *g, *r;
	struct CTCP_Message m;
	gint i = 0;

	strcpy(ctcp_parse_buf, msg+1);
	ctcp_parse_buf[strlen(ctcp_parse_buf)-1] = 0;

	if (!tokenize(ctcp_parse_buf)) return;

	tok_next(&g, &r);

	m.s = s;
	m.source = source;
	m.nick = g_strdup(nick_part(source));
	m.target = target;
	g_strup(g); m.cmd = g;
	m.params = r;

	m.pmask.w_type = 0;
	m.pmask.network = s->fs->Network;
	m.pmask.server = s->fs->Name;
	if (IsChannelName(target)) m.pmask.channel = target; else m.pmask.channel = NULL;
	m.pmask.userhost = source;

	while (CTCP_Handlers[i].Cmd)
	{
		if (!g_strcasecmp(CTCP_Handlers[i].Cmd, g))
		{
			/* <PREFS> Do we have to ignore replies when ignoring CTCP messages ? */
			if (!i || reply || !Ignore_Check(s, source, IGNORE_CTCP))
			{
			/* !i -> when i == 0, ctcp == ACTION, not considered as a CTCP here */
				if (reply) CTCP_Handlers[i].Reply_Function(&m);
				else CTCP_Handlers[i].Request_Function(&m);
			}
			/* <PREFS> Do we have to display ignored CTCP requests (or replies) ? */
			else if (!reply && Ignore_Check(s, source, IGNORE_CTCP))
			{
				if (m.params) VW_output(s->vw_active, T_ERROR, "sF---", s, "CTCP %s (%s) request from %s has been ignored", m.cmd, m.params, m.nick);
				else VW_output(s->vw_active, T_ERROR, "sF--", s, ctcp_tmp,"CTCP %s request from %s has been ignored", m.cmd, m.nick);
			}
			g_free(m.nick);
			return;
		}
		i++;
	}

	if (m.params) VW_output(s->vw_active, T_ERROR, "sF----", s, "Unknown CTCP %s %s from %s (%s)", m.cmd, (reply)? "reply" : "request", m.nick, m.params);
	else VW_output(s->vw_active, T_ERROR, "sF---", s, "Unknown CTCP %s %s from %s", m.cmd, (reply)? "reply" : "request", m.nick);


	g_free(m.nick);
}

/* ----- Requests handlers ------------------------------------------------------------ */

CTCP_HANDLER(CTCP_Ping_Request)
{
	CTCP_Request_Dump(m);
	CTCP_Send(m->s, m->nick, TRUE, m->cmd, m->params, FALSE);
}

CTCP_HANDLER(CTCP_Version_Request)
{
	struct utsname u;
	CTCP_Request_Dump(m);
	if (GPrefs.CTCP_Version) strcpy(ctcp_tmp, GPrefs.CTCP_Version);
	else if (uname(&u)) sprintf(ctcp_tmp, "Ol-Irc:" VER_STRING " (" RELEASE_DATE "):Unknown system"); /* Get Ol-Irc at " WEBSITE " "); */
	else sprintf(ctcp_tmp,"Ol-Irc:" VER_STRING " (" RELEASE_DATE "):%s %s on a %s" /* " Get Ol-Irc at " WEBSITE " " */, u.sysname, u.release, u.machine);
	CTCP_Send(m->s, m->nick, TRUE, m->cmd, ctcp_tmp, FALSE);
}

CTCP_HANDLER(CTCP_Finger_Request)
{
	CTCP_Request_Dump(m);
	if (GPrefs.CTCP_Finger) CTCP_Send(m->s, m->nick, TRUE, m->cmd, GPrefs.CTCP_Finger, FALSE);
	else CTCP_Send(m->s, m->nick, TRUE, m->cmd, m->s->fs->User, FALSE);
}

CTCP_HANDLER(CTCP_Userinfo_Request)
{
	gchar *r;
	CTCP_Request_Dump(m);
	r = prefs_get_gchar(PT_CTCP_USERINFO, &(m->pmask));
	/* if (r) CTCP_Send(m->s, m->nick, TRUE, m->cmd, r, FALSE); */
	if (GPrefs.CTCP_Userinfo) CTCP_Send(m->s, m->nick, TRUE, m->cmd, GPrefs.CTCP_Userinfo, FALSE);
	else CTCP_Send(m->s, m->nick, TRUE, m->cmd, m->s->current_nick, FALSE);
}

CTCP_HANDLER(CTCP_Clientinfo_Request)
{
	CTCP_Request_Dump(m);

	if (GPrefs.CTCP_Clientinfo) strcpy(ctcp_tmp, GPrefs.CTCP_Clientinfo);
	else
	{
		gchar answer[512];
		gint i = 0;
		*answer = 0;
		while (CTCP_Handlers[i].Cmd) strspacecat(answer, CTCP_Handlers[i++].Cmd);
		CTCP_Send(m->s, m->nick, TRUE, m->cmd, answer, FALSE);
	}
}

CTCP_HANDLER(CTCP_Source_Request)
{
	CTCP_Request_Dump(m);
	CTCP_Send(m->s, m->nick, TRUE, m->cmd, WEBSITE, FALSE);
	CTCP_Send(m->s, m->nick, TRUE, m->cmd, NULL, FALSE);
}

CTCP_HANDLER(CTCP_Time_Request)
{
	CTCP_Request_Dump(m);
	if (GPrefs.CTCP_Time) CTCP_Send(m->s, m->nick, TRUE, m->cmd, GPrefs.CTCP_Time, FALSE);
	else
	{
		time_t ti = time(NULL);
		gchar *t = ctime(&ti);
		t[strlen(t)-1] = 0;
		CTCP_Send(m->s, m->nick, TRUE, m->cmd, t, FALSE);
	}
}

CTCP_HANDLER(CTCP_Action_Request)
{
	Query *q;

	if (IsChannelName(m->target))
	{
		Channel *c;
		if (Ignore_Check(m->s, m->source, IGNORE_CHANNEL)) return;
		c = Channel_Find(m->s, m->target);
		if (c) VW_output(c->vw, T_CHAN_ACTION, "snudt", m->s, m->nick, userhost_part(m->source), c->Name, m->params);
		return;
	}

	if (Ignore_Check(m->s, m->source, IGNORE_PRIVATE)) return;
	q = Query_Find(m->s, m->nick);
	if (!q) q = Query_New(m->s, m->nick);
	VW_output(q->vw, T_PRIV_ACTION, "snudt", m->s, m->nick, userhost_part(m->source), m->s->current_nick, m->params);
}

gint CTCP_DCC_Chat_Request(struct CTCP_Message *m)
{
	gchar *g, *r;
	unsigned long IP;
	int Port;

	sprintf(ctcp_tmp,"Bad CTCP DCC CHAT request from %s", m->nick);

	if (!tok_next(&g, &r) || !tok_next(&g, &r)) return T_ERROR;

	sscanf(g, "%lu", &IP);
	Port = atoi(r);

	DCC_Request(DCC_CHAT, m->source, m->s, IP, Port, NULL, 0);

	sprintf(ctcp_tmp, "DCC CHAT Request from %s: %s:%d", m->nick, Dump_IP(IP), Port);
	return T_CTCP_REQUEST;
}

gint CTCP_DCC_Send_Request(struct CTCP_Message *m)
{
	gchar *g, *r;
	gchar *file;
	unsigned long IP;
	guint Port;
	guint32 bytes;

	sprintf(ctcp_tmp,"Bad CTCP DCC SEND request from %s", m->nick);

	if (!tok_next(&g, &r)) return T_ERROR; file = g_strdup(g);
	if (!tok_next(&g, &r)) return T_ERROR; sscanf(g, "%lu", &IP);
	if (!tok_next(&g, &r)) return T_ERROR; Port = atoi(g);
	if (!r) return T_ERROR; bytes = atoi(r);

	DCC_Request(DCC_GET, m->source, m->s, IP, Port, file, bytes);

	sprintf(ctcp_tmp, "DCC Send Request from %s: %s:%d, file %s, %d bytes", m->nick, Dump_IP(IP), Port, file, bytes);
	return T_CTCP_REQUEST;
}

CTCP_HANDLER(CTCP_DCC_Request)
{
	gint t = T_ERROR;

	if (tokenize(m->params))
	{
		gchar *g, *r;

		if (tok_next(&g, &r))
		{
			m->params = r;
			if (!g_strcasecmp(g, "CHAT")) t = CTCP_DCC_Chat_Request(m);
			else if (!g_strcasecmp(g, "SEND")) t = CTCP_DCC_Send_Request(m);
			else sprintf(ctcp_tmp,"Unknown CTCP DCC %s request from %s", g, m->nick);
		}
		else sprintf(ctcp_tmp,"Unknown or bad CTCP DCC %s request from %s", g, m->nick);
	}
	else sprintf(ctcp_tmp,"Bad CTCP %s request from %s", m->cmd, m->nick);

	VW_output(m->s->vw_active, t, "sF-", m->s, "%s", ctcp_tmp);
}

CTCP_HANDLER(CTCP_Request_Dump)
{
	if (irc_cmp(m->target, m->s->current_nick))
		VW_output(m->s->vw_active, T_CTCP_REQUEST, "sF---", m->s, "CTCP %s request from %s to %s", m->cmd, m->nick, m->target);
	else
		VW_output(m->s->vw_active, T_CTCP_REQUEST, "sF--", m->s, "CTCP %s request from %s", m->cmd, m->nick);

}

/* ----- Replies handlers ------------------------------------------------------------- */

CTCP_HANDLER(CTCP_Ping_Reply)
{
	gchar *g, *r;
	time_t t;

	tokenize(m->params);
	tok_next(&g, &r);

	if (g && (t = Get_CTCP_PING_Value(g)))
	{
		time_t tv = time(NULL)-t;
		gchar *plural = (tv > 1)? "s" : "";
		VW_output(m->s->vw_active, T_CTCP_ANSWER, "sF---", m->s, "PING reply from %s: %ld second%s", m->nick, tv, plural);
	}
	else VW_output(m->s->vw_active, T_CTCP_ANSWER, "sF-", m->s, "Bad PING reply from %s", m->nick);
}

CTCP_HANDLER(CTCP_DCC_Reply)
{
}

CTCP_HANDLER(CTCP_Action_Reply)
{
	CTCP_Action_Request(m);
}

CTCP_HANDLER(CTCP_Reply_Dump)
{
	if (!m->params) return;
	
	if (irc_cmp(m->target, m->s->current_nick))
		VW_output(m->s->vw_active, T_CTCP_ANSWER, "sF----", m->s, "CTCP %s reply from %s to %s: %s", m->cmd, m->nick, m->target, m->params);
	else
		VW_output(m->s->vw_active, T_CTCP_ANSWER, "sF---", m->s, "CTCP %s reply from %s: %s", m->cmd, m->nick, m->params);

}

/* vi: set ts=3: */

