
#include "irc.h"

extern void get_networks (GtkWidget *combo, GHashTable *network_hash);

extern void append_to_buffer (IRCServer *server,
	gchar *channel, gchar *text, gchar *colour);

extern void append_to_buffer (IRCServer * server,
	gchar * target, gchar * text, gchar * tag_name);

extern void append_nick_to_buffer (IRCServer *server,
	gchar *target, gchar *nick);


void join_channel (IRCServer *server, gchar *channel_name);

