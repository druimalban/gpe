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

#ifndef __OLIRC_PREFS_H__
#define __OLIRC_PREFS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef enum
{
	PT_ALLOW_CHOOSE_NET,
	PT_SAVE_REAL,
	PT_SAVE_USER,
	PT_SAVE_NICK,
	PT_SAVE_NICK_CHANGES,
	PT_USE_ALT_NICK,
	PT_DISPLAY_PINGS,
	PT_DISPLAY_NOTICES,
	PT_DISPLAY_MOTD,
	PT_DISPLAY_ENDOF,
	PT_DEFAULT_QUIT_REASON,
	PT_QUIT_MENU_ACTION,
	PT_DELETE_EVENT_PRIV_ACTION,
	PT_FONT_NORMAL,
	PT_FONT_BOLD,
	PT_FONT_UNDERLINE,
	PT_FONT_BOLD_UNDERLINE,
	PT_FONT_ENTRY,
	PT_CTCP_FINGER,
	PT_CTCP_VERSION,
	PT_CTCP_CLIENTINFO,
	PT_CTCP_USERINFO,
	PT_CTCP_TIME,
	PT_TAB_POSITION,
	PT_LAST
} OlircPrefType;

struct o_sub_pref
{
	gpointer value;
	struct pmask pmask;
};

struct o_pref
{
	guint flags;
	GList *sub_prefs;
	gchar *keyword;
	gchar *label;
};

extern struct o_pref o_prefs[PT_LAST];

struct o_sub_pref *prefs_get_struct(OlircPrefType type, struct pmask *, gboolean);

gint prefs_get_gint(OlircPrefType type, struct pmask *);
gchar *prefs_get_gchar(OlircPrefType type, struct pmask *);

void prefs_set_value(OlircPrefType, struct pmask *, gpointer);

extern GList *Global_Servers_List;

/* ================================================ */

struct Olirc_Prefs
{
	guint window;
	gchar *network;
	gchar *server;
	gchar *channel;
	gchar *user;
	
	guint32 Colors[16];

	GdkFont *Font_Text_Normal;
	GdkFont *Font_Text_Underline;
	GdkFont *Font_Text_Bold;
	GdkFont *Font_Text_Underline_Bold;
	GdkFont *Font_Entry;

	guint32 Flags;

	gchar *Default_Realname;
	gchar *Default_Quit_Reason;
	gchar *Quit_Reason;

	gchar *Socks_Host;
	guint16 Socks_Port;
	gchar *Socks_User;

	gchar *CTCP_Finger;
	gchar *CTCP_Version;
	gchar *CTCP_Clientinfo;
	gchar *CTCP_Userinfo;
	gchar *CTCP_Time;

	guchar Tab_Pos;
};

extern struct Olirc_Prefs GPrefs;

/* Global Flags Masks */

#define PREF_HISTORIES_SEPARATED (1<<0)
#define PREF_TOOLBAR_SHOW        (1<<1)
#define PREF_USE_SOCKS				(1<<2)

void PreParse_Command_Line(gint, gchar **);
void PostParse_Command_Line(gint, gchar **);
void Prefs_Init(gint, gchar **);
void Prefs_Update();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OLIRC_PREFS_H_ */

/* vi: set ts=3: */

