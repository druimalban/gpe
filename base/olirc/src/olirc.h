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

#ifndef __OLIRC_H__
#define __OLIRC_H__

/* ----- Includes needed everywhere ----- */

#include "config.h"

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef GTK_HAVE_FEATURES_1_1_14
#error "----- Sorry, Ol-Irc now requires Gtk 1.2+... http://www.gtk.org -----" 
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ----- Main defines ----- */

#define VERSION "0"
#define REVISION "0"
#define SUBREVISION "38-alpha-3.2"
#define VER_STRING VERSION "." REVISION "." SUBREVISION
#define RELEASE_DATE "30.11.1999"
#define WEBSITE "http://olirc.hexanet.fr/"
#define MAILTO "olirc@hexanet.fr"

/* ----- Structures ----- */

typedef struct Virtual_Window Virtual_Window;
typedef struct GUI_Window GUI_Window;
typedef struct History History;
typedef struct Channel Channel;
typedef struct Member Member;
typedef struct Server Server;
typedef struct Favorite_Server Favorite_Server;
typedef struct Query Query;
typedef struct Console Console;
typedef struct DCC DCC;

struct Root_Object
{
	GdkColor Colors[16];
	guint32 Flags;
	gchar *username;
	gchar *home_path;
	gchar *directory;
	gchar *logfiles_path;
	gchar *dcc_send_path;
	gchar *dcc_get_path;
	gint main_timer_tag;
};

struct pmask
{
	guint w_type;
	gchar *network;
	gchar *server;
	gchar *channel;
	gchar *userhost;
};

/*	PT_DEFAULT	Use default value
 *	PT_STOP		Stop any further processing (This will be the same as PT_DEFAULT for some prefs, like fonts...)
 *	PT_GINT		Use the gint value given
 *	PT_GCHAR		Use the gchar * value given
 *	PT_SCRIPT	Execute the script and use the value it gives
 */

enum { PT_DEFAULT, PT_STOP, PT_GINT, PT_GCHAR, PT_SCRIPT };

struct pval
{
	guint p_type;
	gint v_gint;
	gchar *v_gchar;
};

struct Server
{
	guint32 object_id;
	Virtual_Window *vw;
	Virtual_Window *vw_active;
	guint32 IP;
	guint16 State;
	gint fd;
	gint gdk_tag;
	Favorite_Server *fs;
	gchar *current_nick;
	GList *Channels;
	GList *Queries;
	GList *Ignores;
	gint offset;
	guint16 Receiving;
	guint16 Flags;
	GtkWidget *Raw_Window;
	GtkWidget *Raw_Text;
	gpointer nick_box;
	gpointer ignore_list_box;
	GList *Callbacks;
	gchar *Input_Buffer;
	gchar *Output_Buffer;
};

struct History
{
	GList *lines;
	guint n_lines;
	guint max_lines;
	GList *pointer;
	gchar *current;
};

struct GUI_Window
{
	guint32 object_id;
	GtkWidget *Gtk_Window;
	GtkWidget *Main_Box;
	GtkWidget *Toolbar_Box;
	GtkWidget *sep1, *sep2;
	GtkWidget *Notebook;
	GtkWidget *Menubar;
	guint nb_pages;
	guint sb_context;
	Virtual_Window *vw_active;
	guint32 last_vw_active;
	GList *vw_list;
	gboolean Toolbar_visible;
	gboolean Menus_visible;
	gboolean Tabs_visible;
	History *Hist;
};

struct Virtual_Window
{
	guint32 object_id;

	struct pmask pmask;

	GUI_Window *rw;
	GtkWidget *Text;
	GtkWidget *Page_Box;
	GtkWidget *Page_Tab_Box;
	GtkWidget *Page_Tab_Label;
	GtkWidget *Page_Tab_Pixmap;
	GtkWidget *TextBox;
	GtkWidget *pane;
	GtkWidget *Scroll;
	GtkWidget *Members;
	GtkWidget *Members_Scrolled;
	GtkWidget *Head_Box;
	GtkWidget *Frame_Box;
	GtkWidget *WF[8], *WFL[8];
	GtkWidget *Topic;
	GtkWidget *entry;

	GtkStyle *style_entry;

	GdkFont *font_entry;
	GdkFont *font_normal;
	GdkFont *font_bold;
	GdkFont *font_underline;
	GdkFont *font_bold_underline;

	GList *Cmd_Index;
	gpointer Member_Index;
	gchar *After;
	gchar *Completing;
	gchar *Before;

	gint Page;
	gboolean Referenced;
	gboolean Empty;
	gboolean Eol;
	signed char Scrolling;
	gchar *Name;
	gchar *Title;
	gpointer Resource;
	History *Hist;
	gint fd_log;
	gint nb_frames;

	GList *lines;

	void (*Close)(gpointer);
	void (*Infos)(gpointer);
	void (*Input)(gpointer, gchar *);
};

struct Channel
{
	guint32 object_id;

	gchar *Name;
	Server *server;
	Virtual_Window *vw;
	GList *Members;
	GList *C_Members;
	guint16 Users;
	guint16 Ops;
	guint16 Voiced;
	gchar *Topic;
	gchar *Mode;
	GList *Bans;
	guint Prefs;
	guint Flags;
	guint16 State;
	gint16 Limit;
	gchar *Key;
};

struct Member
{
	gchar *nick;
	gchar *userhost;
	gint State;
	gint Index;
};

struct Query
{
	guint32 object_id;

	Server *server;
	Virtual_Window *vw;
	Member member;
};

struct Console
{
	Virtual_Window *vw;
};

struct Ignore
{
	gchar *Mask;
	gchar *buf;
	gchar *nick;
	gchar *user;
	gchar *host;
	time_t expiry_date;
	guint16 Flags;
	gchar **channels;
	gpointer ignore_box;
	Server *server;
};

struct Favorite_Server
{
	guint32 object_id;

	gchar *Name;
	gchar *Ports;
	gchar *Location;
	gchar *Network;
	gchar *Password;
	gchar *Nick;
	gchar *User;
	gchar *Real;
	guint32 Flags;
	guint32 Row;
	gpointer properties_box;
};

struct Global_Server
{
	gchar *name;
	gchar *ports;
	gchar *network;
	gchar *location;
};

struct DCC
{
	guint32 object_id;

	guint16 Type;
	guint16 State;
	unsigned long Distant_IP;
	guint16 Distant_Port;
	int fd_socket;	
	int fd_file;
	unsigned long long int size;
	unsigned long long int completed;
	gint gdk_tag;
	Server *server;
	gchar *my_nick;
	Virtual_Window *vw;
	gchar *nick;
	gchar *user;
	gint offset;
	time_t Start_time;	/* 0 before it starts */
	time_t Elapsed_time; /* time of last evaluation */
	time_t End_time;		/* 0 before it ends */
	float Average_speed; /* calculated every 5 seconds */
	gint Row;
	gchar *file;
	GtkDialog *dialog;
	GtkProgressBar *bar;
	GtkLabel *status_label;
	GtkLabel *time_label;
	GtkLabel *completed_label;
	gchar Buffer[4096];
};

/* An IRC Message, parsed */

struct Message
{
	gchar *raw;
	Server *server;
	gchar *command;
	gint c_num;
	gchar *prefix;
	gchar *nick;
	gchar *userhost;
	gint nargs;
	gchar *args[32];
};

#define CALLBACK_LIFETIME 25

#define CBF_KEEP_PROCESSING	(1<<0)
#define CBF_QUIET					(1<<1)

#define DNS_TIMEOUT 15

/* ----- Enumerations and defines ----- */

/* Global Flags */

#define FLAG_MONOCHROME		(1<<0)
#define FLAG_FORK          (1<<1)
#define FLAG_STARTING		(1<<2)
#define FLAG_QUITING			(1<<3)

/* Windows types */

#define W_CONSOLE	(1<<0)
#define W_SERVER	(1<<1)
#define W_CHANNEL	(1<<2)
#define W_QUERY	(1<<3)
#define W_DCC		(1<<4)
#define W_WITH_SERVER (W_SERVER | W_CHANNEL | W_QUERY)
#define W_ALL			 (W_CONSOLE | W_SERVER | W_CHANNEL | W_QUERY | W_DCC)

/* Favorite Server flags */

#define FSERVER_INVISIBLE		(1<<0)
#define FSERVER_WALLOPS  		(1<<1)
#define FSERVER_HIDEMOTD		(1<<2)
#define FSERVER_NOTICES			(1<<3)

/* Server flags */

#define SERVER_SOCKS				(1<<0)
#define SERVER_HIDE				(1<<1)

/* Channel peferences */

#define PREF_CHAN_OP_FIRST     (1<<0)
#define PREF_CHAN_VOICED_FIRST (1<<1)
#define PREF_CHAN_VIEW_JOIN    (1<<2)
#define PREF_CHAN_VIEW_PART    (1<<3)
#define PREF_CHAN_VIEW_QUIT    (1<<4)
#define PREF_CHAN_VIEW_NICK    (1<<5)
#define PREF_CHAN_VIEW_KICK    (1<<6)
#define PREF_CHAN_VIEW_MODE    (1<<7)
#define PREF_CHAN_VIEW_TOPIC   (1<<8)

/* Channel Flags */

#define CHANNEL_EDITING_TOPIC		(1<<0)

/* Control codes masks */

#define CODE_COLOR		(1<<0)
#define CODE_BOLD			(1<<1)
#define CODE_UNDERLINE	(1<<2)
#define CODE_REVERSE		(1<<3)
#define CODE_PLAIN		(1<<4)
#define CODE_BEEP			(1<<5)
#define CODES_ALL			(1<<6)

/* */

enum { HANDLER_BUILTIN = 1, HANDLER_TCL, HANDLER_PYTHON, HANDLER_PERL };

/* ----- Globals variables ----- */

extern struct Root_Object *Olirc;

/* ----- Useful macros ----- */

#define g_free_and_NULL(pointer) G_STMT_START { g_free(pointer); pointer = NULL; } G_STMT_END
#define g_free_and_set(pointer, value) G_STMT_START { g_free(pointer); pointer = value; } G_STMT_END

#define IsChannelName(n) ((n) && (*(n) == '#' || *(n) == '&' || *(n) == '+' || *(n) == '!'))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OLIRC_H__ */

/* vi: set ts=3: */

