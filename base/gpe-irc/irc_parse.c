
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "irc.h"
#include "irc_parse.h"
#include "irc_reply.h"
#include "ctcp.h"
#include "main.h"

#define NICK_MAXLEN			30



/* IRC command parsing functions */
static gboolean irc_cmd_parse_ping (IRCServer * server,
	const gchar * prefix, const gchar * params);

static gboolean irc_cmd_parse_privmsg (IRCServer * server,
	const gchar * prefix, const gchar * params);

static gboolean irc_cmd_parse_notice (IRCServer * server,
	const gchar * prefix, const gchar * params);

static gboolean irc_cmd_parse_nick (IRCServer * server,
	const gchar * prefix, const gchar * params);

static gboolean irc_cmd_parse_join (IRCServer * server,
	const gchar * prefix, const gchar * params);

static gboolean irc_cmd_parse_part (IRCServer * server,
	const gchar * prefix, const gchar * params);

static gboolean irc_cmd_parse_mode (IRCServer *server,
	const gchar * prefix, const gchar * params);




typedef struct
{
    gchar *cmd;
    gboolean (*func) (IRCServer * server,
	    const gchar * prefix, const gchar * params);
}
msg_t;


/* Table for messages from the server. */
msg_t msgtable[] = {
  { "PING",	    irc_cmd_parse_ping },
  { "PRIVMSG",	    irc_cmd_parse_privmsg },
  { "NICK",	    irc_cmd_parse_nick },
  { "JOIN",	    irc_cmd_parse_join },
  { "PART",	    irc_cmd_parse_part },
  { "NOTICE",	    irc_cmd_parse_notice },
  { "MODE",	    irc_cmd_parse_mode},
  { NULL, NULL }
};



gchar *
irc_prefix_to_nick (const gchar * prefix)
{
  gchar *nick = NULL;
  gchar buf[NICK_MAXLEN];

  if (!prefix)
    {
      return NULL;
    }

  if (sscanf (prefix, "%[^!]!%*s", buf) == 1)
    {
      nick = g_strdup (buf);
    }

  return nick;
}


/* Split into 2, g_strfreev the returning gchar ** */
static gchar **
irc_params_split (const gchar * params)
{
  gchar **s;
  if ((s = g_strsplit (params, ":", 2)))
    {
      g_strstrip (s[0]);
    }
  return s;
}


static gboolean
irc_prefix_is_self (IRCServer * server, const gchar * prefix)
{
  gchar *nick;
  gboolean ret = FALSE;

  STRIP_COLON (prefix);
  nick = irc_prefix_to_nick (prefix);

  if (g_ascii_strcasecmp (nick, server->user_info->nick) == 0)
    {
      ret = TRUE;
    }

  g_free (nick);
  return ret;
}


static gboolean
irc_parse_reply (IRCServer * server,
	gchar * prefix, int reply_num, gchar * params)
{
  gchar **str_array = NULL;

  if(!params)
    {
      return FALSE;
    }

  /* The first bit should be nick */
  if (!g_str_has_prefix (params, server->user_info->nick))
    {
      return FALSE;
    }

  params += strlen (server->user_info->nick) + 1;

  STRIP_COLON (params);

  switch (reply_num)
    {
    case IRC_REPLY_FIRST:
      if (!server->prefix && prefix)
	{
	  /* Hopefully the first prefixed message is the server */
	  server->prefix = g_strdup (prefix);
	}
	append_to_buffer_printf (server, NULL, NULL, "%s\n", params);

      break;

    case IRC_REPLY_CHANNEL_TOPIC:
      /* Channel topic */
      {
	str_array = irc_params_split (params);

	IRCChannel *channel =
	  irc_server_channel_get (server, str_array[0]);
	if (channel)
	  {
	    channel->topic = g_strdup (str_array[1]);
	  }

      }
      break;

    case IRC_REPLY_CHANNEL_FOUNDER:
      break;

    case IRC_REPLY_NAMES_DATA:
      /* Could be reply from /names, /join */
      break;

    case IRC_REPLY_NICK_ALREADYINUSE:
      break;

    case IRC_REPLY_NAMES_END:
      /* Some messages that we don't care */
      break;

    default:
      append_to_buffer_printf (server, NULL, NULL, "%s\n", params);
      break;
    }

  g_strfreev (str_array);

  return TRUE;
}


static gboolean
irc_cmd_parse_ping (IRCServer * server,
	const gchar * prefix, const gchar * params)
{
  if (params)
    {
      STRIP_COLON (params);
      irc_pong (server, params);
    }
  return TRUE;
}


static gboolean
irc_cmd_parse_notice (IRCServer * server,
	const gchar * prefix, const gchar * params)
{
  gboolean op_only = FALSE;

  gchar *from = NULL;
  gchar *channel = NULL;
  gchar *msg = NULL;

  gchar **str_array = NULL;

  STRIP_COLON (params);
  str_array = irc_params_split (params);

  from = irc_prefix_to_nick (prefix);

  if (from)
    {
      channel = str_array[0];
      msg = str_array[1];

      if (channel[0] == '@')
	{
	  /* Channel ops only message */
	  op_only = TRUE;
	  channel++;
	}

      if (!IS_CHANNEL(channel))
	{
	  /* User to user notice */
	}

      append_nick_to_buffer (server, channel, from);
    }
  else
    {
      /* If from == NULL, it's probably a server startup NOTICE */
      msg = (char *) params;
    }


  /* TODO: Preference for server window or current window */

  append_to_buffer (server, channel, msg, NULL);
  append_to_buffer (server, channel, "\n", NULL);

  g_free(from);
  g_strfreev (str_array);

  return TRUE;
}


static gboolean
irc_cmd_parse_privmsg (IRCServer * server,
	const gchar * prefix, const gchar * params)
{
  gchar *nick = NULL;
  gchar **str_array = NULL;
//  GString *gstr = g_string_new ("");

  nick = irc_prefix_to_nick (prefix);

  STRIP_COLON (params);
  str_array = irc_params_split (params);

  if (str_array && *str_array && *(str_array + 1))
    {
      if (!ctcp_parse (server, prefix, str_array[0], str_array[1]))
        {
          /* TODO: Parse str_array[0] */
	  append_nick_to_buffer (server, str_array[0], nick);
          append_to_buffer (server, str_array[0], str_array[1], NULL);
          append_to_buffer (server, str_array[0], "\n", NULL);
        }

      g_strfreev (str_array);
    }

  g_free (nick);
//  g_string_free (gstr, TRUE);

  return TRUE;
}


static gboolean
irc_cmd_parse_nick (IRCServer * server,
	const gchar * prefix, const gchar * params)
{
  return TRUE;

}


static gboolean
irc_cmd_parse_join (IRCServer * server,
	const gchar * prefix, const gchar * params)
{
  STRIP_COLON (params);

  if (irc_prefix_is_self (server, prefix))
    {
      /* I just joined a channel! */
      join_channel (server, params);
      append_to_buffer (server, params, "You've ", "tag_nick_ops");
      append_to_buffer (server, params, "joined ", NULL);
      append_to_buffer (server, params, params, "tag_channel");
      append_to_buffer (server, params, "\n", NULL);
    }
  else
    {
      /* Someone else has joined the channel */
      gchar *nick = irc_prefix_to_nick (prefix);
      append_to_buffer (server, params, nick, "tag_nick_ops");
      append_to_buffer (server, params, " has joined ", NULL);
      append_to_buffer (server, params, params, "tag_channel");
      append_to_buffer (server, params, "\n", NULL);
      g_free (nick);
    }

  return TRUE;
}


static gboolean
irc_cmd_parse_mode (IRCServer * server,
	const gchar * prefix, const gchar * params)
{
  gchar *from = NULL;
  gchar *mode_target = NULL;
  gchar *mode_params = NULL;
  gchar **str_array = NULL;

  STRIP_COLON (params);
  str_array = g_strsplit (params, " ", 2);

  if (!str_array)
    {
      return FALSE;
    }

  mode_target = str_array[0];
  mode_params = str_array[1];

  if (!IS_CHANNEL(mode_target))
    {
    }

  from = irc_prefix_to_nick(prefix);

#ifdef DEBUG
  fprintf(stderr, "prefix [%s], from [%s]\n", prefix, from);
#endif

  if(from)
    {
      /* from is valid nick */

      if (IS_CHANNEL(mode_target))
        {
	  /* Channel modes */
	  append_nick_to_buffer (server, mode_target, from);
	  append_to_buffer_printf (server, mode_target, NULL,
		  (const char *) _(" sets mode %s\n"), mode_params);
        }
    }
  else
    {
      /* Not a nick,
       * it should be from a server telling us about channels MODES */
      append_nick_to_buffer (server, mode_target, prefix);
    }


  /* TODO: Keep tracks of channels' and users' modes */

  g_free(from);

  return TRUE;
}



static gboolean
irc_cmd_parse_part (IRCServer * server,
	const gchar * prefix, const gchar * params)
{
  gchar **str_array;

  STRIP_COLON (params);
  str_array = irc_params_split (params);

  if (str_array && *str_array && *(str_array + 1))
    {
      g_strstrip (str_array[0]);
      g_strstrip (str_array[1]);

      if (irc_prefix_is_self (server, prefix))
        {
          /* I just parted a channel! */
          //part_channel(server, str_array[0]);
          append_to_buffer (server, str_array[0], "You've ", "tag_nick_ops");
          append_to_buffer (server, str_array[0], "parted ", NULL);
          append_to_buffer (server, str_array[0], str_array[0],
                            "tag_channel");
        }
      else
        {
          /* Someone else has joined the channel */
          gchar *nick = irc_prefix_to_nick (prefix);
          append_to_buffer (server, str_array[0], nick, "tag_nick_ops");
          append_to_buffer (server, str_array[0], " has parted ", NULL);
          append_to_buffer (server, str_array[0], str_array[0],
                            "tag_channel");
          g_free (nick);
        }

      if (strlen (str_array[1]))
        {
	  append_to_buffer_printf (server,
		  str_array[0], NULL, " (%s)", str_array[1]);
        }

      append_to_buffer (server, str_array[0], "\n", NULL);


    }

  g_strfreev (str_array);

  return TRUE;
}






gboolean
irc_server_parse (IRCServer * server, const gchar * line)
{
  gchar *prefix = NULL;
  gchar *cmd = NULL;
  gchar *params = NULL;

  gchar **str_array = NULL;

  if (!line)
    {
      /* TODO */
      return TRUE;
    }

  if (IS_PREFIXED(line))
    {
      STRIP_COLON (line);
      str_array = g_strsplit (line, " ", 3);
      prefix = g_strdup(str_array[0]);
      cmd = g_strdup(str_array[1]);
      params = g_strdup(str_array[2]);
    }
  else
    {
      str_array = g_strsplit (line, " ", 2);
      cmd = g_strdup(str_array[0]);
      params = g_strdup(str_array[1]);
    }


#ifdef DEBUG
  fprintf(stderr, "irc_server_parse [%s] [%s] [%s]\n",
	  prefix, cmd, params);
#endif

  if (cmd)
    {
      int m = 0;
      gchar *t = NULL;

      /* Look for the function */
      for (; msgtable[m].cmd &&
	   (g_ascii_strcasecmp (msgtable[m].cmd, cmd) != 0); m++);

      /* Remove new line */
      while ((t = strchr (params, '\n')))
	{
	  *t = '\0';
	}

      while ((t = strchr (params, '\r')))
	{
	  *t = '\0';
	}

      if (msgtable[m].func)
	{
	  if (!msgtable[m].func (server, prefix, params))
	    {
	      /* TODO: It failed */
	    }
	}
      else
	{
	  int reply_num = 0;

	  reply_num = atoi (cmd);

	  if (reply_num > 0)
	    {
	      if (!irc_parse_reply (server, prefix, reply_num, params))
		{
		  /* TODO: It failed */
		}
	    }
	}
    }
 
  g_free (prefix);
  g_free (cmd);
  g_free (params);

  g_strfreev (str_array);

  return TRUE;
}
