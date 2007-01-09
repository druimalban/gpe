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

#ifndef __OLIRC_SERVERS_H__
#define __OLIRC_SERVERS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SERVER_INPUT_BUFFER_SIZE 4096
#define SERVER_OUTPUT_BUFFER_SIZE 4096

/* Servers states */

enum { SERVER_IDLE, SERVER_RESOLVING, SERVER_CONNECTING, SERVER_CONNECTED,
		SERVER_DISCONNECTING };

/* Functions */

extern GList *Servers_List;
extern GList *Server_Open_List;

Server *Server_find_by_id(guint32 id);

Server *Server_New(Favorite_Server *fs, gboolean activate, GUI_Window *rw);

extern void Server_Quit(Server *, gchar *);
extern void Server_Disconnect(Server *s, gboolean);
extern void Server_Init(Server *, gchar *);

extern void Servers_Connect_All();
extern void Servers_Reopen();

extern guint Server_Get_Port(Server *);

extern void Server_Display_Raw_Window(Server *);
extern void Server_Raw_Window_Output(Server *, gchar *, gboolean);

/* Server Callbacks */

#define CallbackFunction(c) gboolean (*c)(struct Message *m, gpointer data)

gpointer	Server_Callback_Check(Server *, gint, gchar *, CallbackFunction(c));
gpointer	Server_Callback_Add(Server *, gint, gchar *, gint, CallbackFunction(c), gpointer);
void		Server_Callback_Remove(gpointer);
gboolean	Server_Callback_Call(gpointer, struct Message *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OLIRC_SERVERS_H_ */

/* vi: set ts=3: */

