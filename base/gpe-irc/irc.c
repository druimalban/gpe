/*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <netdb.h>
#include <glib.h>

#include <gtk/gtk.h>

#include "irc.h"
#include "irc_parse.h"

#include "gpe/errorbox.h"

#define SERVICE "ircd"

static GError *gerr = NULL;

static void
handle_gerr (const char *err, GError *gerr)
{
#ifdef DEBUG
    fprintf (stderr, "%s", err);
#endif
    
  if (gerr && gerr->message)
    {
#ifdef DEBUG
      fprintf (stderr, " %s\n", gerr->message);
#endif
      g_error_free (gerr);
    }
#ifdef DEBUG
    else
    {
      fprintf (stderr, " <no gerr>\n");
    }
#endif
}

static gboolean
irc_server_send (IRCServer * server, gchar * command, const gchar * param)
{
  gboolean ret = FALSE;
  gchar *str = NULL;

  if (server)
    {
      str = g_strdup_printf ("%s %s", command, param);

#ifdef DEBUG
      printf ("Sending [%s]\n", str);
#endif

      if ((g_io_channel_write_chars (server->io_channel, str, -1, NULL, &gerr)
           == G_IO_STATUS_NORMAL)
          && (g_io_channel_flush (server->io_channel, NULL)))
        {
          ret = TRUE;
        }
      else
        {
          handle_gerr ("irc_server_send, g_io_channel_write_chars err", gerr);
          ret = FALSE;
        }
      g_free (str);
    }

  return ret;
}

gboolean
irc_pong (IRCServer * server, const gchar * target)
{
  return irc_server_send (server, "PONG", target);
}

gboolean
irc_nick (IRCServer * server, gchar * nick)
{
  return irc_server_send (server, "NICK", nick);
}

gboolean
irc_user (IRCServer * server, gchar * username, gchar * realname)
{
  gboolean ret = FALSE;
  gchar *str;

  str = g_strdup_printf ("%s - - :%s", username, realname);
  ret = irc_server_send (server, "USER", str);
  g_free (str);
  return ret;
}

gboolean
irc_pass (IRCServer * server, const gchar * password)
{
  return irc_server_send (server, "PASS", password);
}

gboolean
irc_join (IRCServer * server, const gchar * channel)
{
  return irc_server_send (server, "JOIN", channel);
}

gboolean
irc_part (IRCServer * server, const gchar * channel, const gchar * reason)
{
  gboolean ret = FALSE;
  gchar *str;

  str = g_strdup_printf ("%s :%s", channel, reason);
  ret = irc_server_send (server, "PART", str);
  g_free (str);
  return ret;
}


gboolean
irc_notice (IRCServer * server, const gchar * target, const gchar * msg)
{
  gboolean ret = FALSE;
  gchar *str;

  str = g_strdup_printf ("%s :%s", target, msg);
  ret = irc_server_send (server, "NOTICE", str);
  g_free (str);
  return ret;
}


gboolean
irc_privmsg (IRCServer * server, const gchar * target, const gchar * msg)
{
  gboolean ret = FALSE;
  gchar *str;

  str = g_strdup_printf ("%s :%s", target, msg);
  ret = irc_server_send (server, "PRIVMSG", str);
  g_free (str);
  return ret;
}

gboolean
irc_action (IRCServer * server, const gchar * target, const gchar * msg)
{
  gboolean ret = FALSE;
  gchar *str;

  str = g_strdup_printf ("ACTION %s", msg);
  ret = irc_privmsg (server, target, str);
  g_free (str);
  return ret;
}

gboolean
irc_quit (IRCServer * server, const gchar * reason)
{
  gboolean ret = FALSE;
  gchar *str;

  str = g_strdup_printf (":%s", reason);
  ret = irc_server_send (server, "QUIT", str);
  //g_io_channel_shutdown (server->io_channel, FALSE, NULL);
  //g_io_channel_unref (server->io_channel);
  g_free (str);
  return ret;
}

gboolean
irc_server_in (GIOChannel * source, GIOCondition condition, gpointer data)
{
  GString *line = NULL;
  GIOStatus status;
  IRCServer *server = data;

  if (condition != G_IO_IN)
    {
#ifdef DEBUG
        fprintf (stderr, "%s:%d woken up, but for %d, not G_IO_IN\n", 
                 __FILE__, __LINE__, condition);
#endif
      return FALSE;
    }

  line = g_string_new ("");
  status = g_io_channel_read_line_string (source, line, NULL, &gerr);

#if 0
  while (status == G_IO_STATUS_NORMAL)
    {
      printf ("[%s]\n", line->str);
      irc_server_parse (server, line->str);
      status = g_io_channel_read_line_string (source, line, NULL, NULL);
    }
#else
  if (status == G_IO_STATUS_NORMAL)
    {
#ifdef DEBUG
      fprintf (stderr, "[%s]\n", line->str);
#endif
      irc_server_parse (server, line->str);
    }
  else
    {
      handle_gerr ("irc_server_in: g_io_channel_read_line err,", gerr);
      return FALSE;
    }
#endif

  g_string_free (line, TRUE);

  return TRUE;
}

gboolean
irc_server_hup (GIOChannel * source, GIOCondition condition, gpointer data)
{
  GIOStatus status;
  IRCServer *server = (IRCServer *) data;

  server->connected = FALSE;
  status = g_io_channel_shutdown (source, FALSE, &gerr);

#ifdef DEBUG
  if (status != G_IO_STATUS_NORMAL)
    {
      handle_gerr ("irc_server_hup: g_io_channel_shutdown err,", gerr);
    }
#endif

  //g_io_channel_unref (source);
  return FALSE;
}

/* Send a the users message to specified channel on specified server */
gboolean
irc_channel_send_message (IRCServer * server, gchar * message)
{
  printf ("IRC Send Message.\n");
  return TRUE;
}

/* Login to the IRC server with the username and password values from server->user_info */
gboolean
irc_server_login (IRCServer * server)
{

  printf ("Sending user info now...\n");

  /*
     irc_privmsg(server, "#gpe", "Testing 123");
     irc_privmsg(server, "pigeon", "Testing 123");
     irc_action(server, "#gpe", "tests gpe-irc");
     irc_part(server, "#gpe", "testing finishes");
   */

  if (server->user_info->password)
    {
      irc_pass (server, server->user_info->password);
    }

  irc_nick (server, server->user_info->nick);
  irc_user (server, server->user_info->username,
            server->user_info->real_name);

  return TRUE;

}

gboolean
irc_server_connect (IRCServer * server)
{
  int fd, connect_result = -1;
  struct addrinfo *address;

  if (getaddrinfo (server->name, SERVICE, NULL, &address) != 0)
    {
      printf ("Unable to obtain info about %s\n", server->name);
      gpe_error_box("Unable to connect!");
      return FALSE;
    }

  printf ("Connecting to %s...", server->name);

  while (address)
    {
      fd =
        socket (address->ai_family, address->ai_socktype,
                address->ai_protocol);

      if (fd != -1)
        {
          connect_result =
            connect (fd, address->ai_addr, address->ai_addrlen);
          if (connect_result != -1)
            {
              server->fd = fd;
              server->io_channel = g_io_channel_unix_new (fd);
              g_io_channel_set_line_term (server->io_channel, "\r\n", -1);
              g_io_add_watch (server->io_channel, G_IO_IN, irc_server_in,
                              server);
              g_io_add_watch (server->io_channel, G_IO_HUP, irc_server_hup,
                              server);
              g_io_channel_set_encoding (server->io_channel, "UTF-8", NULL);
              g_io_channel_set_close_on_unref (server->io_channel, TRUE);
              break;
            }
        }
      address = address->ai_next;
    }

  if (connect_result != -1)
    {
      server->connected = TRUE;
      printf ("Connected.\n");

      if (irc_server_login (server) == TRUE)
        {
          return TRUE;
        }
    }

  printf ("Unable to connect.\n");
  gpe_error_box("Unable to connect!");
  return FALSE;
}

gboolean
irc_server_disconnect (IRCServer * server)
{
  printf ("IRC Server Disconnect.\n");
  return TRUE;
}


IRCChannel *
irc_server_channel_get (IRCServer * server, const gchar * channel_name)
{
  IRCChannel *ircchannel = NULL;

  if (server && server->channel && channel_name && strlen (channel_name))
    {
      ircchannel =
        g_hash_table_lookup (server->channel, (gconstpointer) channel_name);
    }

  return ircchannel;
}
