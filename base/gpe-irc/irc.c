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

#define SERVICE "ircd"

typedef struct
{
  GtkWidget *text_box;
  GtkWidget *entry;
} IRCGtkWidgets;

typedef struct
{
  gchar *nick;
  gchar *username;
  gchar *real_name;
  gchar *email;
  gchar *password;
} IRCUserInfo;

typedef struct
{
  gchar *topic;
  GList *users;
  IRCGtkWidgets *widgets;
  GtkWidget *users_clist;
} IRCChannel;

typedef struct
{
  gchar *name;
  int fd;
  gboolean connected;
  GHashTable *channel;
  IRCUserInfo *user_info;
  IRCGtkWidgets *widgets;
} IRCServer;

gboolean
irc_server_read (IRCServer *server)
{
  fd_set rfds;
  struct timeval tv;
  int data_waiting, buf_len;
  char buf[256];
  char *message;

  FD_ZERO (&rfds);
  FD_SET (server->fd, &rfds);

  if (server->fd == -1)
    printf ("No socket open!\n");

  while (1)
  {
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    printf ("Checking for data...\n");
    data_waiting = select (server->fd + 1, &rfds, NULL, NULL, &tv);

    if (data_waiting)
    {
      printf ("Data waiting.\n");
      buf_len = read (server->fd, buf, sizeof (buf));
      snprintf (message, buf_len, "%s");
      printf ("%s", message);
    }
    else
      sleep (.1);
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
    g_hash_table_insert (server->channel, (gpointer) channel, (gpointer) irc_channel);
    return TRUE;
  }

  printf ("Unable to join channel.\n");
  return FALSE;
}

/* Autojoin any enabled channels */
gboolean
irc_server_login_init (IRCServer *server)
{
  server->channel = g_hash_table_new (g_str_hash, g_str_equal);
  irc_server_join_channel (server, "#gpe");

  irc_server_read (server);
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

int
main (int argc, char *argv[])
{
  IRCServer *server;

  server = g_malloc (sizeof (*server));
  server->user_info = g_malloc (sizeof (*server->user_info));

  if (argc > 2)
  {
    server->name = g_strdup (argv[1]);
    server->user_info->nick = g_strdup (argv[2]);
    server->user_info->username = g_strdup (argv[2]);
    server->user_info->real_name = g_strdup (argv[2]);
    irc_server_connect (server);
  }
  else
  {
    server->name = g_strdup ("irc.handhelds.org");
    server->user_info->nick = g_strdup ("dc_gpe-irc");
    server->user_info->username = g_strdup ("dc_gpe-irc");
    server->user_info->real_name = g_strdup ("dc_gpe-irc");
    irc_server_connect (server);
  }

  return 0;
}

