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
#include <netdb.h>
#include <glib.h>

#include <gtk/gtk.h>

#define SERVICE "irc"

typedef struct
{
  GtkWidget *text_box;
  GtkWidget *entry;
} IRCGtkWidgets;

typedef struct
{
  gchar *nick;
  gchar *real_name;
  gchar *email;
  gchar *password;
} IRCUserInfo;

typedef struct
{
  gchar *channel;
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
  GList *channels;
  IRCUserInfo *user_info;
  IRCGtkWidgets *widgets;
} IRCServer;

gboolean
irc_server_send_message (ICRServer *server, gchar *message)
{
  printf ("IRC Send Message.\n");
  return TRUE;
}

gboolean
irc_server_connect (IRCServer *server)
{
  int fd, connect_result;
  struct addrinfo *address;

  if (getaddrinfo (server->name, SERVICE, NULL, address) != 0)
  {
    printf ("Unable to obtain info about %s\n", server->name);
    return FALSE;
  }

  printf ("Connecting to %s...\n", server->name);

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
    printf ("Connected.\n");
    return TRUE;
  }
  else
  {
    printf ("Uname to connect.\n");
    return FALSE;
  }
}

gboolean
irc_server_disconnect (IRCServer *server)
{
  printf ("IRC Server Disconnect.\n");
  return TRUE;
}

gboolean
irc_server_join_channel (IRCServer *server, gchar *channel)
{
  printf ("IRC Join Channel.\n");
  return TRUE;
}

int
main (int argc, char *argv[])
{
  IRCServer *server;

  if (argc > 1)
  {
    server->name = g_strdup (argv[1]);
    irc_server_connect (server);
  }

  return 0;
}

