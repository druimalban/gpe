
#include "irc.h"

#define _(_x) gettext (_x)

extern void get_networks (GtkWidget *combo, GHashTable *network_hash);


extern void append_to_buffer (IRCServer * server,
	const gchar * target, const gchar * text, const gchar * tag_name);

extern void append_nick_to_buffer (IRCServer *server,
	const gchar *target, const gchar *nick);

extern void append_to_buffer_printf (IRCServer * server,
	const gchar * target, const gchar * tag_name,
	const gchar * format, ...);

void join_channel (IRCServer *server, const gchar *channel_name);

