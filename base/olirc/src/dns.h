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

#ifndef __OLIRC_DNS_H__
#define __OLIRC_DNS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define DNS_Callback(function) void (*function)(struct olirc_hostip *oh, gpointer data)

struct olirc_hostip
{
	gchar *host;
	unsigned long ip;
	gint v_errno;
	gboolean h_error; /* Name resolution error */
	gboolean s_error;	/* System error */
};

void dns_resolve(unsigned long ip, gchar *name, DNS_Callback(callback), gpointer data);

gchar *dns_error(struct olirc_hostip *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OLIRC_DNS_H__ */

/* vi: set ts=3: */

