
#include <time.h>
#include <stdlib.h>

#include "ctcp.h"
#include "irc_parse.h"

#define GPEIRC_NAME			"GPE IRC"
#define GPEIRC_VERSION		"0.1"
#define GPEIRC_INFO			"IRC Client for GPE - http://gpe.handhelds.org/"


static gboolean ctcp_version(IRCServer *server, gchar *prefix, gchar *target, gchar *msg);
static gboolean ctcp_ping(IRCServer *server, gchar *prefix, gchar *target, gchar *msg);
static gboolean ctcp_userinfo(IRCServer *server, gchar *prefix, gchar *target, gchar *msg);
static gboolean ctcp_clientinfo(IRCServer *server, gchar *prefix, gchar *target, gchar *msg);
static gboolean ctcp_finger(IRCServer *server, gchar *prefix, gchar *target, gchar *msg);
static gboolean ctcp_time(IRCServer *server, gchar *prefix, gchar *target, gchar *msg);
static gboolean ctcp_dcc(IRCServer *server, gchar *prefix, gchar *target, gchar *msg);
static gboolean ctcp_action(IRCServer *server, gchar *prefix, gchar *target, gchar *msg);


typedef struct
{
	gchar *ctcp;
	gboolean (* func) (IRCServer *server, gchar *prefix, gchar *target, gchar *msg);
}
ctcp_t;

/* Table for CTCP */
ctcp_t ctcptable[] =
{
	{ "VERSION",	ctcp_version },
	{ "PING",		ctcp_ping },
	{ "USERINFO",	ctcp_userinfo },
	{ "CLIENTINFO",	ctcp_clientinfo },
	{ "FINGER",		ctcp_finger },
	{ "TIME",		ctcp_time },
	{ "DCC",		ctcp_dcc },
	{ "ACTION",		ctcp_action },
	{ NULL, NULL }
};


gboolean
ctcp_send(IRCServer *server, gchar *target, gchar *msg)
{
	gchar *str = NULL;

	str = g_strdup_printf("\x01%s\x01", msg);
	irc_notice(server, target, str);
	g_free(str);
	return TRUE;
}


static gboolean
is_ctcp(gchar *str)
{
	return (str && (str[0] == '\x01') && (str[strlen(str) - 1] == '\x01'));
}



static gboolean
ctcp_version(IRCServer *server, gchar *prefix, gchar *target, gchar *msg)
{
	gchar *nick = NULL;

	nick = irc_prefix_to_nick(prefix);
	ctcp_send(server, nick, GPEIRC_NAME " " GPEIRC_VERSION ", " GPEIRC_INFO);
	g_free(nick);
	return TRUE;
}


static gboolean
ctcp_ping(IRCServer *server, gchar *prefix, gchar *target, gchar *msg)
{
	STRIP_COLON(msg);

	if(msg && strlen(msg) && atoi(msg))
	{
		gchar *nick = NULL;
		gchar *str = NULL;

		nick = irc_prefix_to_nick(prefix);
		str = g_strdup_printf("PING %s", msg);
		ctcp_send(server, nick, str);
		g_free(nick);
		g_free(str);
	}
	return TRUE;
}


static gboolean
ctcp_userinfo(IRCServer *server, gchar *prefix, gchar *target, gchar *msg)
{

	return TRUE;
}


static gboolean
ctcp_clientinfo(IRCServer *server, gchar *prefix, gchar *target, gchar *msg)
{
	int m = 0;
	gchar *nick = NULL;
	gchar *str = NULL;

	nick = irc_prefix_to_nick(prefix);
	str = g_strdup("CLIENTINFO");

	for(; ctcptable[m].ctcp; m++)
	{
		str = g_strconcat(str, " ", ctcptable[m].ctcp, NULL);
	}

	ctcp_send(server, nick, str);
	g_free(nick);
	return TRUE;
}


static gboolean
ctcp_finger(IRCServer *server, gchar *prefix, gchar *target, gchar *msg)
{

	return TRUE;
}


static gboolean
ctcp_time(IRCServer *server, gchar *prefix, gchar *target, gchar *msg)
{
	time_t t;
	gchar *str = NULL;
	gchar *nick = NULL;

	nick = irc_prefix_to_nick(prefix);
	t = time(NULL);
	str = g_strdup_printf("TIME %s", ctime(&t));

	/* Replace the \n from ctime */
	str[strlen(str) - 1] = '\0';

	ctcp_send(server, nick, str);
	g_free(nick);
	g_free(str);

	return TRUE;
}


static gboolean
ctcp_dcc(IRCServer *server, gchar *prefix, gchar *target, gchar *msg)
{

	return TRUE;
}


static gboolean
ctcp_action(IRCServer *server, gchar *prefix, gchar *target, gchar *msg)
{
	/* FIXME: Wait for a better frontend interface */
	gchar *nick = NULL;

	nick = irc_prefix_to_nick(prefix);
	append_to_buffer(server->buffer, "* ", "red");
	append_to_buffer(server->buffer, g_strdup_printf ("%s %s", nick, msg), NULL);
	append_to_buffer(server->buffer, "\n", NULL);

	g_free(nick);
	return TRUE;
}


gboolean
ctcp_parse(IRCServer *server, gchar *prefix, gchar *target, gchar *msg)
{
	gboolean ret = FALSE;

	if(is_ctcp(msg))
	{
		int m = 0;
		gchar **str_array = NULL;

		msg[strlen(msg) - 1] = '\0';
		msg++;

		str_array = g_strsplit(msg, " ", 2);

		if(str_array && *str_array)
		{
			for(; ctcptable[m].ctcp &&
				  (g_ascii_strcasecmp(ctcptable[m].ctcp, str_array[0]) != 0); m++);

			if(ctcptable[m].func)
			{
				ret = ctcptable[m].func(server, prefix, target, str_array[1]);
			}

		}
		else
		{
			/* TODO */
		}

		g_strfreev(str_array);
	}

	return ret;
}


