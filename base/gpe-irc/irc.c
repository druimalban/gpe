/*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <netdb.h>
#include <glib.h>

#include <gtk/gtk.h>

#include "irc.h"

#define SERVICE "ircd"

gboolean
irc_server_read (IRCServer *server, gchar *passback_message)
{
  fd_set rfds;
  int data_waiting, buf_len, char_num = 0, message_num = 0;
  char buf[256];
  char *message = NULL;
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 1;

  FD_ZERO (&rfds);
  FD_SET (server->fd, &rfds);

  if (server->fd == -1)
  {
    printf ("No socket open!\n");
    return FALSE;
  }

  if (!select (server->fd + 1, &rfds, NULL, NULL, &tv))
    return FALSE;

  while (1)
  {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1;
    FD_ZERO (&rfds);
    FD_SET (server->fd, &rfds);
    data_waiting = select (server->fd + 1, &rfds, NULL, NULL, &tv);

    if (data_waiting)
    {
      char_num = 0;
      buf_len = read (server->fd, buf, sizeof (buf));
      if (buf_len != -1)
      {
	while (char_num < buf_len)
	{
	  message = g_realloc (message, message_num + 2);
	  message[message_num] = buf[char_num];

	  message_num++;

          if (buf[char_num] == '\n')
	  {
	    message[message_num] = '\0';
	    printf ("%s", message);
	    message_num = 0;
	    passback_message = g_strdup (message);
	    return TRUE;
	  }

	  char_num++;
	}
      }
    }

    while (gtk_events_pending ())
      gtk_main_iteration ();
  }

  return FALSE;
}

/* Send a the users message to specified channel on specified server */
gboolean
irc_channel_send_message (IRCServer *server, gchar *message)
{
  printf ("IRC Send Message.\n");
  return TRUE;
}

/* Join secified channel on specified server */
gboolean
irc_server_join_channel (IRCServer *server, gchar *channel)
{
  int send_result;
  gchar *join_string;
  IRCChannel *irc_channel;

  printf ("Joining channel %s...", channel);

  join_string = g_strdup_printf ("JOIN %s\r\n", channel);
  send_result = send (server->fd, join_string, strlen (join_string), 0);

  if (send_result != -1)
  {
    printf ("Channel joined.\n");
    irc_channel = g_malloc (sizeof (*irc_channel));
    irc_channel->name = g_strdup (channel);
    irc_channel->queue_in = g_queue_new ();
    g_list_append (server->channels, (gpointer) irc_channel);
    return TRUE;
  }

  printf ("Unable to join channel.\n");
  return FALSE;
}

/* Autojoin any enabled channels */
gboolean
irc_server_login_init (IRCServer *server)
{
  //server->channel = NULL;
  //irc_server_join_channel (server, "#gpe");

  return TRUE;
}

/* Login to the IRC server with the username and password values from server->user_info */
gboolean
irc_server_login (IRCServer *server)
{
  int send_result;
  gchar *login_string;

  if (server->user_info->password)
    login_string = g_strdup_printf ("PASS %s\r\nNICK %s\r\nUSER %s - - :%s\r\n", server->user_info->password, server->user_info->nick, server->user_info->username, server->user_info->real_name);
  else
    login_string = g_strdup_printf ("NICK %s\r\nUSER %s - - :%s\r\n", server->user_info->nick, server->user_info->username, server->user_info->real_name);

  printf ("Now logging in...");

  send_result = send (server->fd, login_string, strlen (login_string), 0);

  if (send_result != -1)
  {
    printf ("Logged in.\n");
    irc_server_login_init (server);
    return TRUE;
  }

  printf ("Login failed.\n");
  return FALSE;
}

gboolean
irc_server_connect (IRCServer *server)
{
  int fd, connect_result;
  struct addrinfo *address;

  if (getaddrinfo (server->name, SERVICE, NULL, &address) != 0)
  {
    printf ("Unable to obtain info about %s\n", server->name);
    return FALSE;
  }

  printf ("Connecting to %s...", server->name);

  while (address)
  {
    fd = socket (address->ai_family, address->ai_socktype, address->ai_protocol);
    if (fd != -1)
    {
      connect_result = connect (fd, address->ai_addr, address->ai_addrlen);
      if (connect_result != -1)
      {
	server->fd = fd;
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

  printf ("Uname to connect.\n");
  return FALSE;
}

gboolean
irc_server_disconnect (IRCServer *server)
{
  printf ("IRC Server Disconnect.\n");
  return TRUE;
}
