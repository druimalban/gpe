
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "irc.h"
#include "irc_parse.h"
#include "ctcp.h"
#include "main.h"

#define NICK_MAXLEN			30


static gboolean irc_parse_ping(IRCServer *server, gchar *prefix, gchar *params);
static gboolean irc_parse_privmsg(IRCServer *server, gchar *prefix, gchar *params);
static gboolean irc_parse_notice(IRCServer *server, gchar *prefix, gchar *params);
static gboolean irc_parse_nick(IRCServer *server, gchar *prefix, gchar *params);

static gboolean irc_parse_join(IRCServer *server, gchar *prefix, gchar *params);
static gboolean irc_parse_part(IRCServer *server, gchar *prefix, gchar *params);

typedef struct
{
	gchar *cmd;
	gboolean (* func) (IRCServer *server, gchar *prefix, gchar *params);
}
msg_t;


/* Table for messages from the server. */
msg_t msgtable[] =
{
	{ "PING",		irc_parse_ping },
	{ "PRIVMSG",	irc_parse_privmsg },
	{ "NICK",		irc_parse_nick },
	{ "JOIN",		irc_parse_join },
	{ "PART",		irc_parse_part },
	{ "NOTICE",		irc_parse_notice },
	{ NULL, NULL }
};



gchar *
irc_prefix_to_nick(gchar *prefix)
{
	gchar *nick = NULL;
	gchar buf[NICK_MAXLEN];

	if(sscanf(prefix, "%[^!]!%*s", buf) == 1)
	{
		nick = g_strdup(buf);
	}

	return nick;
}


/* Split into 2, g_strfreev the returning gchar ** */
static gchar **
irc_params_split(gchar *params)
{
	return g_strsplit(params, ":", 2);
}


static gboolean
irc_prefix_is_self(IRCServer *server, gchar *prefix)
{
	gchar *nick;
	gboolean ret = FALSE;

	STRIP_COLON(prefix);
	nick = irc_prefix_to_nick(prefix);

	if(g_ascii_strcasecmp(nick, server->user_info->nick) == 0)
	{
		ret = TRUE;
	}

	g_free(nick);
	return ret;
}


static gboolean
irc_parse_reply(IRCServer *server, int reply_num, gchar *params)
{
	gchar **str_array = NULL;

	/* The first bit should be nick */
	if(!g_str_has_prefix(params, server->user_info->nick))
	{
		return FALSE;
	}

	params += strlen(server->user_info->nick) + 1;

	str_array = irc_params_split(params);

	if(str_array && *str_array && *(str_array + 1))
	{
		g_strstrip(str_array[0]);

		switch(reply_num)
		{
			case 332:
				/* Channel topic */
				{
					IRCChannel *channel = irc_server_channel_get(server, str_array[0]);
					if(channel)
					{
						channel->topic = g_strdup(str_array[1]);
					}

				}
				break;

			case 333:
				/* Channel founder */
				break;

			case 353:
				/* Channel nick list */
				break;

			case 433:
				/* Duplicate nick */
				break;

			case 366:
				/* Some messages that we don't care */
				/* 366: End of /NAMES list */
				break;

			default:

				break;
		}
		
		g_strfreev(str_array);
	}


	return TRUE;
}


static gboolean
irc_parse_ping(IRCServer *server, gchar *prefix, gchar *params)
{
	if(params)
	{
		STRIP_COLON(params);
		irc_pong(server, params);
	}
	return TRUE;
}

static gboolean
irc_parse_notice(IRCServer *server, gchar *prefix, gchar *params)
{
	return TRUE;
}

static gboolean
irc_parse_privmsg(IRCServer *server, gchar *prefix, gchar *params)
{
	gchar *nick = NULL;
	gchar **str_array = NULL;
	GString *gstr = g_string_new("");

	nick = irc_prefix_to_nick(prefix);

	STRIP_COLON(params);
	str_array = irc_params_split(params);

	if(str_array && *str_array && *(str_array + 1))
	{
		if(!ctcp_parse(server, prefix, str_array[0], str_array[1]))
		{
			/* TODO: Parse str_array[0] */
		  g_strstrip(str_array[0]);
		  append_to_buffer(server, str_array[0], "<", "tag_nick");
		  append_to_buffer(server, str_array[0], nick, NULL);
		  append_to_buffer(server, str_array[0], "> ", "tag_nick");
		  append_to_buffer(server, str_array[0], str_array[1], NULL);
		  append_to_buffer(server, str_array[0], "\n", NULL);
		}

		g_strfreev(str_array);
	}

	g_free(nick);
	g_string_free(gstr, TRUE);

	return TRUE;
}


static gboolean
irc_parse_nick(IRCServer *server, gchar *prefix, gchar *params)
{

	return TRUE;

}


static gboolean
irc_parse_join(IRCServer *server, gchar *prefix, gchar *params)
{
	STRIP_COLON(params);

	if(irc_prefix_is_self(server, prefix))
	{
		/* I just joined a channel! */
		join_channel(server, params);
		append_to_buffer(server, params, "You've ", "tag_nick_ops");
		append_to_buffer(server, params, "joined ", NULL);
		append_to_buffer(server, params, params, "tag_channel");
		append_to_buffer(server, params, "\n", NULL);
	}
	else
	{
		/* Someone else has joined the channel */
		gchar *nick = irc_prefix_to_nick(prefix);
		append_to_buffer(server, params, nick, "tag_nick_ops");
		append_to_buffer(server, params, " has joined ", NULL);
		append_to_buffer(server, params, params, "tag_channel");
		append_to_buffer(server, params, "\n", NULL);
		g_free(nick);
	}

	return TRUE;
}


static gboolean
irc_parse_part(IRCServer *server, gchar *prefix, gchar *params)
{
	gchar **str_array;

	STRIP_COLON(params);
	str_array = irc_params_split(params);

	if(str_array && *str_array && *(str_array + 1))
	{
		g_strstrip(str_array[0]);
		g_strstrip(str_array[1]);

		if(irc_prefix_is_self(server, prefix))
		{
			/* I just parted a channel! */
			//part_channel(server, str_array[0]);
		  append_to_buffer(server, str_array[0], "You've ", "tag_nick_ops");
		  append_to_buffer(server, str_array[0], "parted ", NULL);
		  append_to_buffer(server, str_array[0], str_array[0], "tag_channel");
		}
		else
		{
			/* Someone else has joined the channel */
		  gchar *nick = irc_prefix_to_nick(prefix);
		  append_to_buffer(server, str_array[0], nick, "tag_nick_ops");
		  append_to_buffer(server, str_array[0], " has parted ", NULL);
		  append_to_buffer(server, str_array[0], str_array[0], "tag_channel");
			g_free(nick);
		}

		if(strlen(str_array[1]))
		{
		  append_to_buffer(server, str_array[0], g_strdup_printf(" (%s)", str_array[1]), NULL);
		}

		append_to_buffer(server, str_array[0], "\n", NULL);


	}

	g_strfreev(str_array);

	return TRUE;
}






gboolean
irc_server_parse(IRCServer *server, gchar *line)
{
	gchar *prefix = NULL;
	gchar *cmd = NULL;
	gchar *params = NULL;

	gchar **str_array = NULL;
	gchar **tmp = NULL;
	int cnt = 0;

	if(!line)
	{
		/* TODO */
		return TRUE;
	}

	if(line[0] == ':')
	{
		line++;
	}
	else
	{
		/* No prefix */
		cnt++;
	}

	str_array = g_strsplit(line, " ", 3);

	if(str_array && *str_array)
	{
		for(tmp = str_array; *tmp != NULL; tmp++, cnt++)
		{
			switch(cnt)
			{
				case 0:
					prefix = g_strdup(*tmp);
					break;
				case 1:
					cmd = g_strdup(*tmp);
					break;
				case 2:
					params = g_strdup(*tmp);
					break;
				default:
					break;
			}
		}


		if(cmd)
		{
			int m = 0;
			gchar *t = NULL;

			/* Look for the function */
			for(; msgtable[m].cmd &&
				  (g_ascii_strcasecmp(msgtable[m].cmd, cmd) != 0); m++);

			/* Remove new line */
			while((t = strchr(params, '\n')))
			{
				*t = '\0';
			}

			while((t = strchr(params, '\r')))
			{
				*t = '\0';
			}

			if(msgtable[m].func)
			{
				if(!msgtable[m].func(server, prefix, params))
				{
					/* TODO: It failed */
				}
			}
			else
			{
				if(!server->prefix && prefix)
				{
					/* Hopefully the first prefixed message is the server */
					server->prefix = g_strdup(prefix);
				}

				if(atoi(cmd))
				{
					if(!irc_parse_reply(server, atoi(cmd), params))
					{
						/* TODO: It failed */
					}
				}
			}
		}

		g_free(prefix);
		g_free(cmd);
		g_free(params);

	}

	g_strfreev(str_array);

	return TRUE;
}
