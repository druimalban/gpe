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

#ifndef __OLIRC_ICONS_H__
#define __OLIRC_ICONS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern char *Server_xpm[];
extern char *prefs_xpm[];

extern char *console_xpm_data[];
extern char *channel_xpm_data[];
extern char *query_xpm_data[];
extern char *server_xpm_data[];
extern char *dcc_chat_xpm_data[];

extern GdkPixmap *console_xpm;
extern GdkBitmap *console_xpm_mask;

extern GdkPixmap *channel_xpm;
extern GdkBitmap *channel_xpm_mask;

extern GdkPixmap *query_xpm;
extern GdkBitmap *query_xpm_mask;

extern GdkPixmap *server_xpm;
extern GdkBitmap *server_xpm_mask;

extern GdkPixmap *dcc_chat_xpm;
extern GdkBitmap *dcc_chat_xpm_mask;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OLIRC_ICONS_H_ */

/* vi: set ts=3: */

