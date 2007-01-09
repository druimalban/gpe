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

#ifndef __OLIRC_CHANNELS_H__
#define __OLIRC_CHANNELS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Channel members status */

#define FLAG_VOICE  (1<<0)
#define FLAG_OP     (1<<1)
#define FLAG_IRCOP  (1<<2)

#define HAS_OP(m) ((m)->State & FLAG_OP)
#define HAS_VOICE(m) ((m)->State & FLAG_VOICE)

/* Channels states */

typedef enum { CHANNEL_NOT_JOINED, CHANNEL_BANNED, CHANNEL_KICKED, CHANNEL_JOINED,
		 CHANNEL_JOINING, CHANNEL_REJOINING, CHANNEL_INVITE_ONLY, CHANNEL_FULL,
		 CHANNEL_UNAVAILABLE, CHANNEL_BAD_KEY, CHANNEL_TOOMANYCHANS } OlircChannelState;

/* Function */

extern Channel *Channel_find_by_id(guint32);

extern GList *Channel_Open_List;

extern void Channel_Join(Channel *, gchar *);
extern void Channel_Part(Channel *, gchar *);

extern Channel *Channel_New(Server *, gchar *);
extern Channel *Channel_Find(Server *, gchar *);

extern void Channel_Topic_Focus_In(GtkWidget *, GdkEvent *, gpointer);
extern void Channel_Topic_Focus_Out(GtkWidget *, GdkEvent *, gpointer);

extern void Channel_Msg_Join(Server *, gchar *, gchar *);
extern void Channel_Msg_Part(Server *, gchar *, gchar *, gchar *);
extern void Channel_Msg_Names(Server *, gchar *, gchar *);
extern void Channel_Msg_Privmsg(Server *, gchar *, gchar *, gchar *);
extern void Channel_Msg_Notice(Server *, gchar *, gchar *, gchar *);

extern void Channel_Msg_Op(Channel *, gchar *, gchar *, gboolean);
extern void Channel_Msg_Voice(Channel *, gchar *, gchar *, gboolean);
extern void Channel_Msg_Ban(Channel *, gchar *, gchar *, gboolean);
extern void Channel_Msg_Mode(Channel *, gchar *, gchar *, gboolean);
extern void Channel_Msg_Key(Channel *c, gchar *, gchar *, gboolean);
extern void Channel_Msg_Limit(Channel *c, gchar *, gchar *, gboolean);
extern void Channel_Msg_Flag(Channel *c, gchar *, gchar, gboolean);

/* Management of members */

extern Member *Channel_Member_Find(Channel *, gchar *);
extern void Channel_Member_Update(Channel *, Member *, gchar *, gint);

extern void Channels_Member_Userhost_Store(Server *, gchar *);

extern void Channel_Members_ClearList(Channel *);
void Channel_Members_Reset(Channel *);

extern gboolean Channel_Add_User(Channel *, gchar *);
extern gboolean Channel_Remove_User(Channel *, gchar *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OLIRC_CHANNELS_H_ */

/* vi: set ts=3: */

