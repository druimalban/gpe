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
  gchar *topic;
  GList *users;
} IRCChannel;

typedef struct
{
  gchar *name;
  GString *text;
  int fd;
  gboolean connected;
  GHashTable *channel;
  IRCUserInfo *user_info;
} IRCServer;

extern gchar *irc_server_read (IRCServer *server);

extern gboolean irc_channel_send_message (IRCServer *server, gchar *message);

extern gboolean irc_server_join_channel (IRCServer *server, gchar *channel);

extern gboolean irc_server_login (IRCServer *server);

extern gboolean irc_server_connect (IRCServer *server);

extern gboolean irc_server_disconnect (IRCServer *server);
