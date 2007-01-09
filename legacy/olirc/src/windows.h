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

#ifndef __OLIRC_WINDOWS_H__
#define __OLIRC_WINDOWS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Texts types */

enum {	T_PRIV_MSG,				/* msg received from an user (generally in a query) */
			T_PRIV_ACTION,			/* action received from an user (generally in a query) */
			T_PRIV_NOTICE,			/* notice received from an user */
			T_CHAN_MSG,				/* msg received in a channel */
			T_CHAN_ACTION,			/* action received in a channel */
			T_CHAN_NOTICE,			/* notice received in a channel */
			T_DCC_MSG,				/* msg received in a dcc */
			T_DCC_ACTION,			/* action received in a dcc */

			T_OWN_MSG,				/* you sending a msg in a query */
			T_OWN_ACTION,			/* you sending an action in a query */
			T_OWN_CHAN_MSG,		/* you sending a msg in a channel */
			T_OWN_CHAN_ACTION,	/* you sending an action in a channel */
			T_OWN_DCC_MSG,			/* you sending a msg in a dcc */
			T_OWN_DCC_ACTION,		/* you sending an action in a dcc */

			T_OWN_SEND_MSG,		/* you sending a /msg to a nick/channel */
			T_OWN_SEND_NOTICE,	/* you sending a /notice to a nick/channel */

			T_WARNING,
			T_NORMAL,
			T_JOIN,					/* someone joingning a channel */
			T_PART,					/* someone leaving a channel */
			T_QUIT,					/* someone leaving IRC */
			T_KICK,					/* someone kicked from a channel */
			T_MODE,
			T_NAMES,
			T_TOPIC,					/* channel topic is / channel topic change */
			T_ERROR,
			T_NICK,					/* a nick change */
			T_INVITE,
			T_CTCP_SEND,
			T_CTCP_REQUEST,
			T_CTCP_ANSWER,
			T_ANSWER,

			T_OWN_NICK,				/* you changing nick */
			T_OWN_JOIN,				/* you joigning a channel */
			T_OWN_PART,				/* you leaving a channel */
			T_OWN_KICK,				/* you beeing kicked from a channel */
			T_OWN_QUIT				/* you quiting IRC */

			/* and much more to add */
		 };

struct w_line
{
	time_t date;
	guint type;
	gchar *args_type;
	gpointer *args[8];
	gboolean new;
};

/* Global variables */

extern GList *VW_List;

/* Functions */

void Sync();

/* Virtual_Windows */

Virtual_Window *VW_new();
Virtual_Window *VW_find_by_id(guint32);

void VW_load_fonts();
void VW_unload_fonts();
void VW_reload_prefs();

void VW_output(Virtual_Window *, guint, gchar *, ...);
void VW_Close(Virtual_Window *);
void VW_Remove_From_GW(Virtual_Window *, gboolean);
void VW_Add_To_GW(Virtual_Window *, GUI_Window *);
void VW_Activate(Virtual_Window *);
void VW_Clear(Virtual_Window *);
void VW_Status(Virtual_Window *);

void VW_entry_insert(Virtual_Window *, gchar *);

void VW_Scroll_Up(Virtual_Window *, gboolean);
void VW_Scroll_Down(Virtual_Window *, gboolean);

void VW_Init(Virtual_Window *, GUI_Window *, gint);

void VW_Move_To_GW(Virtual_Window *, GUI_Window *);

/* Real Windows */

extern GList *GW_List;

GUI_Window *GW_New(gboolean);
GUI_Window *GW_find_by_id(guint32);

void GW_Windows_List_Switch(GUI_Window *);

gboolean GW_Activate_Last_VW(GUI_Window *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OLIRC_WINDOWS_H__ */

/* vi: set ts=3: */

