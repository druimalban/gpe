#ifndef CTCP_H
#define CTCP_H

#include "irc.h"

gboolean ctcp_parse(IRCServer *server,
	const gchar *prefix, const gchar *target, const gchar *msg);

#endif

