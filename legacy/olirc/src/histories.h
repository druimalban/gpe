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

#ifndef __OLIRC_HISTORIES_H__
#define __OLIRC_HISTORIES_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void History_Add(Virtual_Window *, gchar *);
void History_Prev(Virtual_Window *);
void History_Next(Virtual_Window *);
void History_Clear(History *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OLIRC_HISTORIES_H_ */


/*
vim: ts=3
*/

/* vi: set ts=3: */

