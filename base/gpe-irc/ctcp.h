#ifndef CTCP_H
#define CTCP_H

#include "irc.h"

gboolean ctcp_parse(IRCServer *server, gchar *prefix, gchar *target, gchar *msg);

#endif

