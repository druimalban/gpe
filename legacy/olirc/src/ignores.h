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

#ifndef __OLIRC_IGNORES_H__
#define __OLIRC_IGNORES_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Ignores masks */

#define IGNORE_PRIVATE	(1<<0)
#define IGNORE_CHANNEL	(1<<1)
#define IGNORE_INVITE	(1<<2)
#define IGNORE_NOTICE	(1<<3)
#define IGNORE_CTCP		(1<<4)
#define IGNORE_EXCLUDE	(1<<5)

/* Functions */

extern void dialog_ignore_properties(Server *, struct Ignore *, Member *, gchar *);
extern void dialog_unignore(Server *, Member *);

extern void Ignore_Expire(struct Ignore *);
extern gboolean Ignore_Check(Server *, gchar *, guint);

extern void Ignore_List_New(Server *);

extern void Server_Ignores_Destroy(Server *);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OLIRC_IGNORES_H__ */

/* vi: set ts=3: */

