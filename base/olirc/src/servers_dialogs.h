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

#ifndef __OLIRC_SERVERS_DIALOGS_H__
#define __OLIRC_SERVERS_DIALOGS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern GList *Favorite_Servers_List;

Favorite_Server *favorite_server_find(gchar *);
Favorite_Server *favorite_server_add_or_update(gboolean, Favorite_Server *fs, gchar *name, gchar *ports, gchar *net, gchar *loc, gchar *nick, gchar *user, gchar *real, gchar *pwd, guint32 flags);
void favorite_network_add(gchar *);

void dialog_favorite_servers();
void dialog_server_properties(Favorite_Server *, Server *, GUI_Window *);

void dialog_server_nick(Server *, gchar *, gchar *);

void dialog_quit_servers(GList *, gchar *, Virtual_Window *, GUI_Window *, gboolean);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OLIRC_SERVERS_DIALOGS_H_ */

/* vi: set ts=3: */

