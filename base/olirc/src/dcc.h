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

#ifndef __OLIRC_DCC_H__
#define __OLIRC_DCC_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* DCC Types */

enum { DCC_CHAT, DCC_SEND, DCC_GET };

/* DCC States */

enum { DCC_LISTENING, DCC_CONNECTING, DCC_ACTIVE, DCC_FAILED, DCC_FINISHED, DCC_ABORTED };

/* Functions */

extern GList *DCC_List;

extern void DCC_Request(guint, gchar *, Server *, unsigned long, guint, gchar *, guint32);
extern DCC *DCC_Send_New(gchar *, Server *, gchar *);
extern DCC *DCC_Chat_New(gchar *, Server *, unsigned long, gint);

extern void DCC_Connections_Update(DCC *, gboolean);
extern void DCC_Display_Connections();

extern void DCC_Send_Filesel(Server *, gchar *);

extern void DCC_Box_Update(DCC *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OLIRC_DCC_H_ */

/* vi: set ts=3: */

