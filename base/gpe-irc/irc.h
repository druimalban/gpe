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
  gchar *topic;
  GList *users;
  GQueue *queue_in;
} IRCChannel;

typedef struct
{
  gchar *name;
  int fd;
  gboolean connected;
  GList *channels;
  GQueue *queue_in;
  IRCUserInfo *user_info;
} IRCServer;

extern gboolean irc_server_read (IRCServer *server, gchar *passback_message);

extern gboolean irc_channel_send_message (IRCServer *server, gchar *message);

extern gboolean irc_server_join_channel (IRCServer *server, gchar *channel);

extern gboolean irc_server_login (IRCServer *server);

extern gboolean irc_server_connect (IRCServer *server);

extern gboolean irc_server_disconnect (IRCServer *server);
