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

#ifdef USE_TCL

#include "olirc.h"
#include "tcl-olirc.h"
#include "windows.h"
#include "servers.h"
#include "network.h"
#include "channels.h"
#include "misc.h"
#include "entry.h"

#include <string.h>

Tcl_Interp *olirc_tcl_interp;

gchar tcl_tmp[2048];
gchar tcl_tmp_2[2048];

#define T_HANDLER(f) gint f(ClientData cd, Tcl_Interp *interp, int argc, char **argv)

#define T_CHECK_ARGS(min, max, help) \
  if ((argc<(min)) || (argc>(max))) { \
    Tcl_AppendResult(interp,"Wrong # args: should be \"",argv[0], (help),"\"",NULL); \
    return TCL_ERROR; \
  }

#define T_ERROR(string) \
{ \
	Tcl_AppendResult(interp, string, NULL); \
	return TCL_ERROR; \
}

#define T_RETURN_STRING(str) \
{ \
	Tcl_AppendResult(interp, str, NULL); \
	return TCL_OK; \
}

#define T_RETURN_UINT(prefix, v) \
{ \
	sprintf(tcl_tmp, "%s%lu", prefix, (unsigned long int) v); \
	Tcl_AppendResult(interp, tcl_tmp, NULL); \
	return TCL_OK; \
}

#define T_RETURN_INT(prefix, v) \
{ \
	sprintf(tcl_tmp, "%s%ld", prefix, (long int) v); \
	Tcl_AppendResult(interp, tcl_tmp, NULL); \
	return TCL_OK; \
}

T_HANDLER(tcl_vwindow);
T_HANDLER(tcl_server);
T_HANDLER(tcl_channel);
T_HANDLER(tcl_query);
T_HANDLER(tcl_dcc);
T_HANDLER(tcl_olirc);
T_HANDLER(tcl_add_command);

void tcl_init(Virtual_Window *vw)
{
	GList *l;
	int i = 0;

	/* Adds our commands to the TCL interpreter */

	Tcl_CreateCommand(olirc_tcl_interp, "vwindow", tcl_vwindow, NULL, NULL);
	Tcl_CreateCommand(olirc_tcl_interp, "server", tcl_server, NULL, NULL);
	Tcl_CreateCommand(olirc_tcl_interp, "channel", tcl_channel, NULL, NULL);
	Tcl_CreateCommand(olirc_tcl_interp, "query", tcl_query, NULL, NULL);
	Tcl_CreateCommand(olirc_tcl_interp, "dcc", tcl_dcc, NULL, NULL);
	Tcl_CreateCommand(olirc_tcl_interp, "olirc", tcl_olirc, NULL, NULL);
	Tcl_CreateCommand(olirc_tcl_interp, "add_command", tcl_add_command, NULL, NULL);

	/* Looks for TCL scripts that should be autoloaded */

	VW_output(vw, T_NORMAL, "F-", "Autoloading TCL scripts from %s*.tcl ...", Olirc->directory);

	l = find_files(Olirc->directory, ".tcl");

	while (l)
	{
		sprintf(tcl_tmp_2, "source %s%s", Olirc->directory, (gchar *) l->data);
		tcl_do(vw, tcl_tmp_2);
		g_free(l->data);
		l = l->next;
		i++;
	}

	VW_output(vw, T_NORMAL, "F-", "TCL scripts loaded : %d", i);
}

void tcl_do(Virtual_Window *vw, gchar *script)
{
	int ret;

	g_return_if_fail(vw);
	g_return_if_fail(script);

	sprintf(tcl_tmp, "vw%lu", (unsigned long int) vw->object_id);
	Tcl_SetVar(olirc_tcl_interp, "current_vwindow", tcl_tmp, TCL_GLOBAL_ONLY);

	ret = Tcl_Eval(olirc_tcl_interp, script);

	if (!vw) return;

	if (ret == TCL_OK)
	{
		if (*(olirc_tcl_interp->result))
			VW_output(vw, T_NORMAL, "F", "TCL Return: %s", olirc_tcl_interp->result);
	}
	else VW_output(vw, T_WARNING, "F", "TCL Error: %s", olirc_tcl_interp->result);
}

T_HANDLER(tcl_vwindow)
{
	static gchar *commands[] = { "list", "output", "close", "title", "name", "activate", "type",
		"server", "channel", "query", "dcc", "clear", "" };

	guint32 vw_id;
	Virtual_Window *vw = NULL;
	gchar *f;
	gint i;

	T_CHECK_ARGS(2, 999, " command ...");

	i = 0; while (*(commands[i]) && g_strcasecmp(argv[1], commands[i])) i++;

	if (!*(commands[i])) T_ERROR("Unknown vwindow subcommand");

	if (i == 0)	/* list */
	{
		GList *l = VW_List;

		while (l)
		{
			sprintf(tcl_tmp, "vw%d", ((Virtual_Window *) l->data)->object_id);
  			Tcl_AppendElement(interp, tcl_tmp);
			l = l->next;
		}

		return TCL_OK;
	}

	if (!argv[2]) T_ERROR("No window identifier specified");

	if (sscanf(argv[2], "vw%lu", (unsigned long int *) &vw_id) == 1)
		vw = VW_find_by_id(vw_id);

	if (vw == NULL) T_ERROR("Bad window identifier");

	switch (i)
	{
		case 1:	/* output */
			if (argv[3] == 0) T_ERROR("No string specified");
			if (!strcmp(argv[3], "-nohilight")) f = "#t"; else f = "t";
			VW_output(vw, T_NORMAL, f, argv[3]);
			break;

		case 2:	/* close */
			VW_Close(vw);
			break;

		case 3:	/* name */
			T_RETURN_STRING(vw->Name);

		case 4:	/* title */
			T_RETURN_STRING(vw->Title);

		case 5:	/* activate */
			VW_Activate(vw);
			break;

		case 6:	/* type */
			T_RETURN_INT("", vw->pmask.w_type);

		case 7:	/* server */
			if (vw->pmask.w_type == W_SERVER)
				T_RETURN_UINT("sv", ((Server *) vw->Resource)->object_id);
			break;

		case 8:	/* channel */
			if (vw->pmask.w_type == W_CHANNEL)
				T_RETURN_UINT("ch", ((Channel *) vw->Resource)->object_id);
			break;

		case 9:	/*	query */
			if (vw->pmask.w_type == W_QUERY)
				T_RETURN_UINT("qu", ((Query *) vw->Resource)->object_id);
			break;

		case 10:	/* dcc */
			if (vw->pmask.w_type == W_DCC)
				T_RETURN_UINT("dc", ((DCC *) vw->Resource)->object_id);
			break;

		case 11:	/* clear */
			VW_Clear(vw);
			return TCL_OK;
	}

	return TCL_OK;
}

T_HANDLER(tcl_server)
{
	gint i;
	static const gchar *commands[] = { "list", "status", "connect", "disconnect", "channels", 
		"output", "name", "network", "queries", "dcc", "quit", "" };
	Server *s = NULL;
	guint32 sv_id;

	T_CHECK_ARGS(2, 999, " command ...");

	i = 0; while (*(commands[i]) && g_strcasecmp(argv[1], commands[i])) i++;

	if (!*(commands[i])) T_ERROR("Unknown server subcommand");

	if (i == 0)	/* list */
	{
		GList *l = Servers_List;

		while (l)
		{
			sprintf(tcl_tmp, "sv%d", ((Server *) l->data)->object_id);
  			Tcl_AppendElement(interp, tcl_tmp);
			l = l->next;
		}

		return TCL_OK;
	}

	if (!argv[2]) T_ERROR("No server identifier specified");

	if (sscanf(argv[2], "sv%lu", (unsigned long int *) &sv_id) == 1)
		s = Server_find_by_id(sv_id);

	if (s == NULL) T_ERROR("Bad server identifier");

	switch (i)
	{
		case 1:	T_RETURN_INT("", s->State);	/* state */

		case 2:	/* connect */
			if (s->State != SERVER_IDLE) T_ERROR("Already connected to the server.");
			Server_Connect(s);
			return TCL_OK;

		case 3:	/* disconnect */
			if (s->State == SERVER_IDLE) T_ERROR("Not connected to the server.");
			Server_Disconnect(s, TRUE);
			return TCL_OK;

		case 4:	/* channels */
		{
			GList *l = s->Channels;
			while (l)
			{
				sprintf(tcl_tmp, "ch%d", ((Channel *) l->data)->object_id);
	  			Tcl_AppendElement(interp, tcl_tmp);
				l = l->next;
			}
			break;
		}

		case 5:	/* output */
			if (s->State != SERVER_CONNECTED) T_ERROR("Server not connected");
			Server_Output(s, argv[3], TRUE);
			break;

		case 6:	T_RETURN_STRING(s->fs->Name);		/* Name */
		case 7:	T_RETURN_STRING(s->fs->Network);	/* Network */

		case 8:	/* queries */
		{
			GList *l = s->Queries;
			while (l)
			{
				sprintf(tcl_tmp, "qu%d", ((Query *) l->data)->object_id);
	  			Tcl_AppendElement(interp, tcl_tmp);
				l = l->next;
			}
			break;
		}


		case 10:	/* quit */
			if (!argv[3]) T_ERROR("QuitReason not specified (can't use default QuitReason yet :/)");
			if (s->State == SERVER_IDLE) T_ERROR("Not connected to the server.");
			Server_Quit(s, argv[3]);
			return TCL_OK;

	}

	return TCL_OK;
}

T_HANDLER(tcl_channel)
{
	gint i;
	static const gchar *commands[] = { "name", "server", "vwindow", "topic", "mode", 
		"privmsg", "notice", "part", "join", "" };
	Channel *c = NULL;
	guint32 ch_id;

	T_CHECK_ARGS(2, 999, " command ...");

	i = 0; while (*(commands[i]) && g_strcasecmp(argv[1], commands[i])) i++;

	if (!*(commands[i])) T_ERROR("Unknown channel subcommand");

	if (!argv[2]) T_ERROR("No channel identifier specified");

	if (sscanf(argv[2], "ch%lu", (unsigned long int *) &ch_id) == 1)
		c = Channel_find_by_id(ch_id);

	if (c == NULL) T_ERROR("Bad channel identifier");

	switch (i)
	{
		case 0:	T_RETURN_STRING(c->Name);	/* name */
		case 1:	T_RETURN_UINT("sv", c->server->object_id);	/* server */
		case 2:	T_RETURN_UINT("vw", c->vw->object_id);	/* vwindow */
		case 3:	T_RETURN_STRING(c->Topic);	/* topic */
		case 4:	T_RETURN_STRING(c->Mode);	/* mode */

		case 5:	/* privmsg */
			if (!argv[3]) T_ERROR("No string specified");
			if (c->server->State == SERVER_IDLE) T_ERROR("Server not connected");
			sprintf(tcl_tmp, "PRIVMSG %s :%s", c->Name, argv[3]);
			Server_Output(c->server, tcl_tmp, TRUE);
			sprintf(tcl_tmp, "%s!userhost", c->server->current_nick);
			Channel_Msg_Privmsg(c->server, tcl_tmp, c->Name, argv[3]);
			return TCL_OK;

		case 6:	/* notice */
			if (!argv[3]) T_ERROR("No string specified");
			if (c->server->State == SERVER_IDLE) T_ERROR("Server not connected");
			sprintf(tcl_tmp, "NOTICE %s :%s", c->Name, argv[3]);
			Server_Output(c->server, tcl_tmp, TRUE);
			sprintf(tcl_tmp, "%s!userhost", c->server->current_nick);
			Channel_Msg_Notice(c->server, tcl_tmp, c->Name, argv[3]);
			return TCL_OK;

		case 7:	/* part */
			if (c->server->State == SERVER_IDLE) T_ERROR("Server not connected");
			if (argv[3]) sprintf(tcl_tmp, "PART %s :%s", c->Name, argv[3]);
			else sprintf(tcl_tmp, "PART %s", c->Name);
			Server_Output(c->server, tcl_tmp, TRUE);
			return TCL_OK;

		case 8:	/* join */
			if (c->server->State == SERVER_IDLE) T_ERROR("Server not connected");
			if (argv[3]) sprintf(tcl_tmp, "JOIN %s %s", c->Name, argv[3]);
			else sprintf(tcl_tmp, "JOIN %s", c->Name);
			Server_Output(c->server, tcl_tmp, TRUE);
			return TCL_OK;

	}

	return TCL_OK;
}

T_HANDLER(tcl_query)
{
	T_CHECK_ARGS(2, 999, " command ...");

	return TCL_OK;
}

T_HANDLER(tcl_dcc)
{
	T_CHECK_ARGS(2, 999, " command ...");

	return TCL_OK;
}

T_HANDLER(tcl_olirc)
{
	T_CHECK_ARGS(2, 999, " command ...");

	return TCL_OK;
}

T_HANDLER(tcl_add_command)
{
	T_CHECK_ARGS(3, 3, " command function");

	parser_add_command(argv[1], W_ALL, HANDLER_TCL, argv[2], 0, -1, "help string");

	return TCL_OK;
}

#endif	/* USE_TCL */

/* vi: set ts=3: */

