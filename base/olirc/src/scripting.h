/*
 * ol-irc - A small irc client using GTK+
 *
 * Copyright (C) 1998, 1999 Yann Grossel [Olrick]
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef __OLIRC_SCRIPTING_H__
#define __OLIRC_SCRIPTING_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
	EV_FIRST = 0,
	EV_SERVER_CONNECTED,
	EV_SERVER_QUIT,
	EV_SERVER_DISCONNECTED,
	EV_SERVER_NOTICE,
	EV_CHANNEL_JOIN,
	EV_CHANNEL_PART,
	EV_CHANNEL_KICK,
	EV_CHANNEL_MODE,
	EV_CHANNEL_FLAG,
	EV_CHANNEL_OP,
	EV_CHANNEL_DEOP,
	EV_CHANNEL_VOICE,
	EV_CHANNEL_DEVOICE,
	EV_CHANNEL_BAN,
	EV_CHANNEL_UNBAN,
	EV_CHANNEL_LIMIT,
	EV_CHANNEL_KEY,
	EV_CHANNEL_TOPIC,
	EV_CHANNEL_PRIVMSG,
	EV_CHANNEL_NOTICE,
	EV_CHANNEL_CTCP,
	EV_USER_MODE,
	EV_USER_NICK,
	EV_USER_PRIVMSG,
	EV_USER_NOTICE,
	EV_USER_INVITE,
	EV_USER_CTCP_REQUEST,
	EV_USER_CTCP_REPLY,
	EV_LAST						/* Must be the last event type */
} OlircEventType;

struct OEvent
{
	OlircEventType type;
	gchar *network;
	gchar *server;
	gchar *channel;
	gchar *nickuser;
	gchar *target;
	gchar *string;
};

void Event_Bind(OlircEventType type, gchar *network, gchar *server, gchar *channel, gchar *nickuser, gchar *target, gchar *string, gchar *function);
gboolean Event_Unbind(OlircEventType type, gchar *network, gchar *server, gchar *channel, gchar *nickuser, gchar *target, gchar *string, gchar *function);
gboolean Event_Raise(OlircEventType type, Virtual_Window *vw, Server *s, Channel *c, gchar *nickuser, gchar *target, gchar *string);

void Script_Init(gint, gchar **, Virtual_Window *);

gboolean olirc_timer(gpointer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OLIRC_SCRIPTING_H_ */

/* vi: set ts=3: */

