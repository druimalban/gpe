#ifndef IRC_PARSE_H
#define IRC_PARSE_H

#include "irc.h"

/* This simply does a pointer increment, use it carefully */
#define STRIP_COLON(x)		if(x[0] == ':') x++

#define IS_CHANNEL(x)		(x[0] == '#' ? TRUE : FALSE)



gboolean irc_server_parse(IRCServer *server, gchar *line);

/* Returns a nick from a full prefix, free required */
gchar *irc_prefix_to_nick(gchar *prefix);

#endif

