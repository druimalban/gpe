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

// #define DEBUG_SCRIPTING

#include "olirc.h"
#include "scripting.h"
#include "dcc.h"
#include "ignores.h"
#include "servers.h"

#ifdef USE_PYTHON
#include "python-olirc.h"
#endif

#ifdef USE_TCL
#include "tcl-olirc.h"
#endif

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>

GList *Event_Bindings[EV_LAST];
gchar *Event_Name[EV_LAST];

struct { OlircEventType type; gchar *name; } Event_Names[] =
{
	{ EV_SERVER_CONNECTED, "EV_Server_Connected" },
	{ EV_SERVER_QUIT, "EV_Server_Quit" },
	{ EV_SERVER_DISCONNECTED, "EV_Server_Disconnected" },
	{ EV_SERVER_NOTICE, "EV_Server_Notice" },
	{ EV_CHANNEL_JOIN, "EV_Channel_Join" },
	{ EV_CHANNEL_PART, "EV_Channel_Part" },
	{ EV_CHANNEL_KICK, "EV_Channel_Kick" },
	{ EV_CHANNEL_MODE, "EV_Channel_Mode" },
	{ EV_CHANNEL_FLAG, "EV_Channel_Flag" },
	{ EV_CHANNEL_OP, "EV_Channel_Op" },
	{ EV_CHANNEL_DEOP, "EV_Channel_Deop" },
	{ EV_CHANNEL_VOICE, "EV_Channel_Voice" },
	{ EV_CHANNEL_DEVOICE, "EV_Channel_Devoice" },
	{ EV_CHANNEL_BAN, "EV_Channel_Ban" },
	{ EV_CHANNEL_UNBAN, "EV_Channel_Unban" },
	{ EV_CHANNEL_TOPIC, "EV_Channel_Topic" },
	{ EV_CHANNEL_PRIVMSG, "EV_Channel_Privmsg" },
	{ EV_CHANNEL_NOTICE, "EV_Channel_Notice" },
	{ EV_CHANNEL_CTCP, "EV_Channel_Ctcp" },
	{ EV_CHANNEL_KEY, "EV_Channel_Key" },
	{ EV_CHANNEL_LIMIT, "EV_Channel_Limit" },
	{ EV_USER_MODE, "EV_User_Mode" },
	{ EV_USER_NICK, "EV_User_Nick" },
	{ EV_USER_PRIVMSG, "EV_User_Privmsg" },
	{ EV_USER_NOTICE, "EV_User_Notice" },
	{ EV_USER_INVITE, "EV_User_Invite" },
	{ EV_USER_CTCP_REQUEST, "EV_User_CTCP_Request" },
	{ EV_USER_CTCP_REPLY, "EV_User_CTCP_Reply" },
	{ 0, NULL }
};

void Script_Init(gint argc, gchar **argv, Virtual_Window *vw)
{
	gint i = 0;

#ifdef USE_PYTHON

	FILE *fp;
	gchar tmp[256], buf[1024];

#endif

#ifdef USE_TCL

	olirc_tcl_interp = Tcl_CreateInterp();
	Tcl_Init(olirc_tcl_interp);
	Tcl_SetVar(olirc_tcl_interp, "tcl_interactive", "0", TCL_GLOBAL_ONLY);
	tcl_init(vw);

#endif

#ifdef USE_PYTHON

	Init_Olirc_Py(argc, argv);	/* Initialize the Python Interpreter and the olirc module */

	/* Execute the python script init.py */

	if ((fp = fopen("init.py", "r")))
	{
		PyRun_SimpleFile(fp, "init.py");
		fclose(fp);
	}

	PyRun_SimpleString("print 'Initializing Python...'\n");

	/* Define the event types constants in the python interpreter */

	*buf = 0;

	while (Event_Names[i].name)
	{
		Event_Name[Event_Names[i].type] = Event_Names[i].name;
		sprintf(tmp, "%s = %d\n", Event_Names[i].name, (gint) Event_Names[i].type);
		if (strlen(buf) + strlen(tmp) + 2 > sizeof(buf))
		{
			PyRun_SimpleString(buf);
			*buf = 0;
		}
		strcat(buf, tmp);
		i++;
	}

	if (*buf) PyRun_SimpleString(buf);

	/* Read the functions defined by the user */

	PyRun_SimpleString("print 'Parsing user functions...'\n");

	if ((fp = fopen("functions.py", "r")))
	{
		PyRun_SimpleFile(fp, "functions.py");
		fclose(fp);
	}

	/* Binds the functions to events */

	PyRun_SimpleString("print 'Parsing user bindings...'\n");

	if ((fp = fopen("bindings.py", "r")))
	{
		PyRun_SimpleFile(fp, "bindings.py");
		fclose(fp);
	}

	PyRun_SimpleString("print 'Done.'\n");

#endif

	i = 0;

	while (Event_Names[i].name)
	{
		Event_Name[Event_Names[i].type] = Event_Names[i].name;
		i++;
	}
}

gboolean Event_Raise(OlircEventType type, Virtual_Window *vw, Server *s, Channel *c, gchar *nickuser, gchar *target, gchar *string)
{
	GList *l;

	#ifdef DEBUG_SCRIPTING
	fprintf(stdout, "Event raised : type = %s vw = %s server = %s channel = %s nickuser = %s target = %s string = %s\n",
		Event_Name[type], (vw)? vw->Name : "(none)", (s)? s->ls->Name : "(none)", (c)? c->Name : "(none)", (nickuser)? nickuser : "(none)", (target)? target : "(none)", (string)? string : "(none)");
	#endif

	g_return_val_if_fail( (type > 0) && (type < EV_LAST), FALSE);

	if (! (l = Event_Bindings[type]) ) return FALSE;

	return FALSE;
}

/* ----- Main timer function called every second -------------------------------------- */

gboolean olirc_timer(gpointer data)
{
	register GList *l, *m;
	struct Ignore *ign;

	/* Remove Ignores that have expired */

	l = Servers_List;

	while (l)
	{
		m = ((Server *) l->data)->Ignores;

		while (m)
		{
			ign = (struct Ignore *) m->data;
			m = m->next;
			if (ign->expiry_date && ign->expiry_date <= time(NULL)) Ignore_Expire(ign);
		}

		l = l->next;
	}

#ifdef USE_DCC
	/* Update all DCC Send/Get boxes */

	l = DCC_List;

	while (l)
	{
		if (((DCC *) l->data)->Type == DCC_SEND || ((DCC *) l->data)->Type == DCC_GET)
			DCC_Box_Update((DCC *) l->data);
		l = l->next;
	}
#endif

	return TRUE;
}

/* vi: set ts=3: */

