#ifndef IRC_H
#define IRC_H

#include <gtk/gtk.h>

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
  gchar *name;
  GString *text;
  gchar *prefix;
  int fd;
  GIOChannel *io_channel;
  gboolean connected;
  GHashTable *channel;
  GtkWidget *button;
  IRCUserInfo *user_info;
} IRCServer;

typedef struct
{
  gchar *name;
  IRCServer *server;
  GString *text;
  gchar *topic;
  GtkWidget *button;
  GList *users;
} IRCChannel;

enum irc_type
{
  IRC_SERVER,
  IRC_CHANNEL
};


extern gboolean irc_server_login (IRCServer *server);
extern gboolean irc_server_connect (IRCServer *server);
extern gboolean irc_server_disconnect (IRCServer *server);

/* Message */
extern gboolean irc_privmsg (IRCServer *server, gchar *target, gchar *msg);
extern gboolean irc_notice (IRCServer *server, gchar *target, gchar *msg);
extern gboolean irc_quit (IRCServer *server, gchar *reason);
extern gboolean irc_join (IRCServer *server, gchar *channel);
extern gboolean irc_part (IRCServer *server, gchar *channel, gchar *reason);
extern gboolean irc_pong (IRCServer *server, gchar *target);



#endif

