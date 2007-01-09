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

#ifndef __OLIRC_MISC_H__
#define __OLIRC_MISC_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <string.h>

#define trim_spaces(a) g_strstrip(g_strdup(a))

#define strspacecat(d,s) G_STMT_START { if (*d) strcat(d, " "); strcat(d, s); } G_STMT_END

gchar *strip_spaces(gchar *);

gboolean is_string_empty(gchar *);
gboolean is_string_an_IP(gchar *);

unsigned long convert_IP(gchar *);

gint irc_cmp(gchar *,  gchar *);
gint irc_ncmp(gchar *,  gchar *, guint);
gchar *nick_part(gchar *);
gchar *ident_part(gchar *);
gchar *userhost_part(gchar *);
gchar *network_from_hostname(gchar *);
gchar *tokenize(gchar *);
gchar *tok_next(gchar **, gchar **);

gchar *irc_date(time_t);

gchar *Dump_IP(unsigned long IP);

gchar *Strip_Codes(gchar *, guint);

gchar *Remember_Filepath(GtkFileSelection *, gchar **);

gboolean mask_match(gchar *, gchar *);

gchar *Duration(unsigned long);

GList *find_files(gchar *, gchar *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OLIRC_MISC_H_ */

/* vi: set ts=3: */

