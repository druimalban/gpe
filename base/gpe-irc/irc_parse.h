#ifndef IRC_PARSE_H
#define IRC_PARSE_H

#include "irc.h"

#define STRIP_COLON(x)		if(x[0] == ':') x++

gboolean irc_server_parse(IRCServer *server, gchar *line);

gchar *irc_prefix_to_nick(gchar *prefix);

#endif

