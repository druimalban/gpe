#include "irc.h"

extern void get_networks (GtkWidget *combo, GHashTable *network_hash);
extern void append_to_buffer (GtkTextBuffer *buffer, gchar *text, gchar *colour);
void join_channel (IRCServer *server, gchar *channel_name);
