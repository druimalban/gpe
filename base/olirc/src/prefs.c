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

/* Management of preferences : Command line parsing, and Loading/Saving prefs files */

#include "olirc.h"
#include "prefs.h"
#include "windows.h"
#include "misc.h"
#include "servers.h"
#include "servers_dialogs.h"
#include "channels.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

GList *Global_Servers_List = NULL;

struct Olirc_Prefs GPrefs;

static gchar *GtkPos_Names[] = { "Left", "Right", "Top", "Bottom" };

gchar ptmp[4096];

/* ------------------------------------------------------------------------------------- */

#define PREF_GINT		(1<<0)
#define PREF_STRING	(1<<1)

struct o_pref o_prefs[PT_LAST];

struct
{
	OlircPrefType p_type;
	guint flags;
	gchar *keyword;
	gchar *label;
	gpointer value;
} def_prefs[] =
{
	{ PT_DEFAULT_QUIT_REASON, PREF_STRING, "default_quit_reason", "", NULL },
	{ PT_QUIT_MENU_ACTION, PREF_GINT, "action_quit_menu", "", (gpointer) 0 },
	{ PT_DELETE_EVENT_PRIV_ACTION, PREF_GINT, "action_delete_event", "", (gpointer) 0 },
	{ PT_FONT_ENTRY, PREF_STRING, "font_entry", "Entry font", "-adobe-helvetica-medium-r-normal-*-14-*-*-*-p-*-iso8859-1" },
	{ PT_FONT_NORMAL, PREF_STRING, "font_normal", "Normal font", "-adobe-courier-medium-r-normal-*-12-*-*-*-m-*-iso8859-1" },
	{ PT_FONT_BOLD, PREF_STRING, "font_bold", "Bold font", "-adobe-courier-bold-r-normal-*-12-*-*-*-m-*-iso8859-1" },
	{ PT_FONT_UNDERLINE, PREF_STRING, "font_underline", "Underline font", "-adobe-courier-medium-o-normal-*-12-*-*-*-m-*-iso8859-1" },
	{ PT_FONT_BOLD_UNDERLINE, PREF_STRING, "font_bold_underline", "Bold + Underline font", "-adobe-courier-bold-o-normal-*-12-*-*-*-m-*-iso8859-1" },
	{ PT_CTCP_FINGER, PREF_STRING, "ctcp_finger", "Ctcp Finger reply", NULL },
	{ PT_CTCP_VERSION, PREF_STRING, "ctcp_version", "Ctcp Version reply", NULL },
	{ PT_CTCP_CLIENTINFO, PREF_STRING, "ctcp_clientinfo", "Ctcp Clientinfo reply", NULL },
	{ PT_CTCP_USERINFO, PREF_STRING, "ctcp_userinfo", "Ctcp Userinfo reply", NULL },
	{ PT_CTCP_TIME, PREF_STRING, "ctcp_time", "Ctcp Time reply", NULL },
/*	{ PT_TAB_POSITION, PREF_GINT, "tab_position", "Tab Position", 2 }, */
	{ PT_LAST, 0, NULL, NULL }
};

void prefs_init()
{
	gint i;
	struct o_pref *op;
	struct o_sub_pref *osp;

	i = 0;

	while (def_prefs[i].p_type != PT_LAST)
	{
		op = &(o_prefs[def_prefs[i].p_type]);
		op->flags = def_prefs[i].flags;
		osp = (struct o_sub_pref *) g_malloc0(sizeof(struct o_sub_pref));
		if (op->flags & PREF_STRING) osp->value = (gpointer) g_strdup((gchar *) def_prefs[i].value);
		else osp->value = def_prefs[i].value;
		op->sub_prefs = g_list_append(NULL, (gpointer) osp);
		op->keyword = def_prefs[i].keyword;
		op->label = def_prefs[i].label;
		i++;
	}
}

struct o_sub_pref *prefs_get_struct(guint type, struct pmask *pmask, gboolean exact_match)
{
	struct o_sub_pref *osp;
	GList *l;
	gint score_v, score_c;
	struct o_sub_pref *s_osp;
	gint s_score_v, s_score_c;
	gchar *sk;

	g_return_val_if_fail(type < PT_LAST, NULL);
	if (!(o_prefs[type].sub_prefs)) return NULL;

	/* If no mask, we return the default preference struct */

	if (!pmask) return (struct o_sub_pref *) o_prefs[type].sub_prefs->data;

	// if (type == PT_CTCP_USERINFO) { printf("Preference %s cherchée : type = %s, w_type = %d, network = %s, server = %s, channel = %s, userhost = %s\n", exact_match? "EXACTEMENT" : "", o_prefs[type].keyword, pmask->w_type, pmask->network, pmask->server, pmask->channel, pmask->userhost); }

	l = o_prefs[type].sub_prefs;

	s_osp = (struct o_sub_pref *) o_prefs[type].sub_prefs->data;
	s_score_v = 0; s_score_c = 0;

	while (l)
	{
		score_v = 0; score_c = 0;

		osp = (struct o_sub_pref *) l->data; l = l->next;

		// if (type == PT_CTCP_USERINFO) { printf("On teste contre [ %d %s %s %s %s ]\n", osp->pmask.w_type, osp->pmask.network, osp->pmask.server, osp->pmask.channel, osp->pmask.userhost); }

		if (exact_match)
		{
			if (osp->pmask.w_type != pmask->w_type) continue;

			if (osp->pmask.network && pmask->network) { if (g_strcasecmp(osp->pmask.network, pmask->network)) continue; }
			else if (osp->pmask.network || pmask->network) continue;

			if (osp->pmask.server && pmask->server) { if (g_strcasecmp(osp->pmask.server, pmask->server)) continue; }
			else if (osp->pmask.server || pmask->server) continue;

			if (osp->pmask.channel && pmask->channel) { if (g_strcasecmp(osp->pmask.channel, pmask->channel)) continue; }
			else if (osp->pmask.channel || pmask->channel) continue;

			if (osp->pmask.userhost && pmask->userhost) { if (g_strcasecmp(osp->pmask.userhost, pmask->userhost)) continue; }
			else if (osp->pmask.userhost || pmask->userhost) continue;

			// if (type == PT_CTCP_USERINFO) printf("Trouvé un masque exactement identique\n\n");

			return osp;
		}
		else
		{
			if (pmask->w_type)
			{
				if (!(osp->pmask.w_type & pmask->w_type)) continue;
				score_v++;
			}

			sk = osp->pmask.network;
			if (sk && pmask->network && mask_match(sk, pmask->network))
			{ score_v++; while (*sk) { if (*sk != '*' && *sk != '?') score_c++; sk++; } }
			else if (sk) continue;

			sk = osp->pmask.server;
			if (sk && pmask->server && mask_match(sk, pmask->server))
			{ score_v++; while (*sk) { if (*sk != '*' && *sk != '?') score_c++; sk++; } }
			else if (sk) continue;

			sk = osp->pmask.channel;
			if (sk && pmask->channel && mask_match(sk, pmask->channel))
			{ score_v++; while (*sk) { if (*sk != '*' && *sk != '?') score_c++; sk++; } }
			else if (sk) continue;

			sk = osp->pmask.userhost;
			if (sk && pmask->userhost && mask_match(sk, pmask->userhost))
			{ score_v++; while (*sk) { if (*sk != '*' && *sk != '?') score_c++; sk++; } }
			else if (sk) continue;
		}

		// if (type == PT_CTCP_USERINFO) { printf(" Score : items : %d, chars: %d\n", score_v, score_c); }

		if (score_v > s_score_v || (score_v == s_score_v && score_c > s_score_c))
		{
			s_score_v = score_v;
			s_score_c = score_c;
			s_osp = osp;
		}
	}

	// if (type == PT_CTCP_USERINFO) printf("\n");

	if (exact_match) return NULL;

	return s_osp;
}

gint prefs_get_gint(guint type, struct pmask *pmask)
{
	struct o_sub_pref *osp;
	g_return_val_if_fail(type < PT_LAST, 0);
	if (!(o_prefs[type].sub_prefs)) return 0;
	g_return_val_if_fail(o_prefs[type].flags & PREF_GINT, 0);
	osp = prefs_get_struct(type, pmask, FALSE);
	return (gint) ((osp)? osp->value : 0);
}

gchar *prefs_get_gchar(guint type, struct pmask *pmask)
{
	struct o_sub_pref *osp;
	g_return_val_if_fail(type < PT_LAST, NULL);
	if (!(o_prefs[type].sub_prefs)) return NULL;
	g_return_val_if_fail(o_prefs[type].flags & PREF_STRING, NULL);
	osp = prefs_get_struct(type, pmask, FALSE);
	return (gchar *) ((osp)? osp->value : NULL);
}

void prefs_set_value(guint type, struct pmask *pmask, gpointer value)
{
	struct o_sub_pref *osp;
	g_return_if_fail(type < PT_LAST);
	osp = prefs_get_struct(type, pmask, TRUE);
	if (!osp)
	{
		osp = (struct o_sub_pref *) g_malloc0(sizeof(struct o_sub_pref));
		osp->pmask.w_type = pmask->w_type;
		osp->pmask.network = pmask->network;
		osp->pmask.server = pmask->server;
		osp->pmask.channel = pmask->channel;
		osp->pmask.userhost = pmask->userhost;
		o_prefs[type].sub_prefs = g_list_append(o_prefs[type].sub_prefs, (gpointer) osp);
	}
	osp->value = value;
}

/* ----- Global and Favorite Servers --------------------------------------------------- */

void global_servers_read()
{
	/* Read the predefined servers */
	/* Fill the Global_Servers_List with them */

	struct Global_Server *gs;

	/* Servers are hardcoded, but I'll put them in a text file as soon as I
	 * implement the 'install' option in the Makefile (as soon as I switch
	 * to automake in fact)
	 */

	struct Global_Server gbs[] =
	{
		{ "irc.emn.fr", "6667", "IrcNet", "France, Nantes" },
		{ "irc.eurecom.fr", "6667", "IrcNet", "France, Nice" },
		{ "irc.enst.fr", "6667", "IrcNet", "France, Paris" },
		{ "irc.insat.com", "6667", "IrcNet", "France, Paris" },
		{ "sil.polytechnique.fr", "6667", "IrcNet", "France, Paris" },
		{ "irc.twiny.net", "6667", "IrcNet", "France, Paris" },
		{ "irc.grolier.fr", "6666-6669,7000", "IrcNet", "France, Paris" },
		{ "irc.funet.fi", "6667", "IrcNet", "Finland" },
		{ "chat.bt.net", "6667,6679", "IrcNet", "United Kingdom, London" },
		{ NULL, NULL, NULL, NULL }
	};

	gint i = 0;

	while (gbs[i].name)
	{
		gs = (struct Global_Server *) g_malloc0(sizeof(struct Global_Server));
		gs->name = gbs[i].name;
		gs->ports = gbs[i].ports;
		gs->network = gbs[i].network;
		gs->location = gbs[i].location;
		Global_Servers_List = g_list_append(Global_Servers_List, (gpointer) gs);
		i++;
	}

}

/* ------------------------------------------------------------------------------------- */

void Print_Help(gchar *pname, gint ret)
{
	fprintf(stdout, "\nThis is Ol-Irc %s, a small irc client.\n", VER_STRING);
	fprintf(stdout, "\nUsage: %s [options]\n", pname);
	fprintf(stdout, "\n");
	fprintf(stdout, "-v, --version   display the version of Ol-Irc and exit.\n");
	fprintf(stdout, "-h, --help      print this help.\n");
	fprintf(stdout, "    --license   display details about the GNU Public License, then exit.\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "-m, --mono      force Ol-Irc to use only black and white colors.\n");
	fprintf(stdout, "-f, --fork      makes Ol-Irc to fork() on startup.\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "Much more options will come soon.\n");
	fprintf(stdout, "\n");
	exit(ret);
}

void Print_Version(gboolean full)
{
	if (full)
	{
		fprintf(stdout, "Ol-irc %s, released %s\n\n", VER_STRING, RELEASE_DATE);
		fprintf(stdout, "Copyright (C) 1998, 1999 Yann Grossel.\n\n");
	}
	else fprintf(stdout, "Ol-irc %s\n", VER_STRING);
	exit(0);
}

void Print_License()
{
	fprintf(stdout, " Ol-Irc is copyright (C) 1998, 1999 Yann Grossel [Olrick].\n\n");
	fprintf(stdout, " This program is free software; you can redistribute it and/or modify\n"
	" it under the terms of the GNU General Public License as published by\n the Free Software Foundation; either version 2 of the License, or\n"
	" (at your option) any later version.\n\n This program is distributed in the hope that it will be useful,\n"
	" but WITHOUT ANY WARRANTY; without even the implied warranty of\n MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	" GNU General Public License for more details.\n\n"
	" You should have received a copy of the GNU General Public License\n along with this program; if not, write to the Free Software\n"
	" Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n\n");

	exit(0);
}

void PreParse_Command_Line(gint argc, gchar **argv)
{
	gint i;

	if (argc>1) for (i=1; i<argc; i++)
	{
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) Print_Help(argv[0], 0);
		else if (!strcmp(argv[i], "-v")) Print_Version(FALSE);
		else if (!strcmp(argv[i], "--version")) Print_Version(TRUE);
		else if (!strcmp(argv[i], "--license")) Print_License();
		else if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--fork")) Olirc->Flags |= FLAG_FORK;
	}
}

void PostParse_Command_Line(gint argc, gchar **argv)
{
	gint i;

	if (argc>1) for (i=1; i<argc; i++)
	{
		if (!strcmp(argv[i], "-m") || !strcmp(argv[i], "--mono")) Olirc->Flags |= FLAG_MONOCHROME;
		else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) continue;
		else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) continue;
		else if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--fork")) continue;
		else
		{
			fprintf(stderr, "\n%s: Unknown option '%s'\n\nTry %s --help.\n\n", argv[0], argv[i], argv[0]);
			gtk_exit(1);
		}
	}
}

/* ------------------------------------------------------------------------------------- */

gchar *next_string(gchar **s)
{
	register gchar *seek = *s;
	register gchar *w, *r;
	gchar *start;

	while (*seek && (*seek == ' ' || *seek == '\t')) seek++;
	if (!*seek || *seek == '\n') return NULL;

	if (*seek == '\"')
	{
		start = ++seek;
		while (*seek && *seek != '\n' && (*seek != '\"' || *(seek-1) == '\\')) seek++;
		if (!*seek || *seek == '\n') return NULL;
		*seek++ = 0;
	}
	else
	{
		start = seek;
		while (*seek && *seek != '\n' && *seek != ' ' && *seek != '\t') seek++;
		if (*seek) *seek++ = 0;
	}

	w = r = start;
	while (*r) { if (*r == '\\' && r[1] == '\"') r++; *w++ = *r++; }
	*w = 0;
	*s = seek;
	return start;
}

void dump_string(FILE *f, gchar *s, gboolean need_quotes)
{
	static gchar safe_buf[2048];
	register gchar *r, *w;
	register gint count = 0;

	r = s;

	if (r && strlen(r) >= sizeof(safe_buf)) g_warning("String too long !!");

	w = safe_buf; *w++ = '\"';

	if (!r || !*r) need_quotes = TRUE;
	else while (*r && ++count<sizeof(safe_buf))
	{
		if (*r == ' ' || *r == '\t') need_quotes = TRUE;
		else if (*r == '\"') { need_quotes = TRUE; *w++ = '\\'; }
		*w++ = *r++;
	}

	if (need_quotes) { *w++ = '\"'; *w = 0; fprintf(f, " %s", safe_buf); }
	else { *w = 0; fprintf(f, " %s", safe_buf + 1); }
}

gboolean Prefs_Add_FS(gchar *seek)
{
	gchar *name, *port, *loc, *net, *pwd, *nick, *user, *real, *flags;
	if (!(name = next_string(&seek))) return FALSE;
	if (!(port = next_string(&seek))) return FALSE;
	if (!(net = next_string(&seek))) return FALSE;
	if (!(loc = next_string(&seek))) return FALSE;
	if (!(nick = next_string(&seek))) return FALSE;
	if (!(user = next_string(&seek))) return FALSE;
	if (!(real = next_string(&seek))) return FALSE;
	if (!(flags = next_string(&seek))) return FALSE;
	if (!(pwd = next_string(&seek))) return FALSE;

	favorite_server_add_or_update(FALSE, NULL, name, port, net, loc, nick, user, real, pwd, atoi(flags));

	return TRUE;
}

gboolean prefs_read(gchar *keyword, gint size, gchar *seek)
{
	gint i;
	gchar *sk, *st;
	struct pmask pmask;

	for (i = 0; i < PT_LAST; i++)
	{
		if (!o_prefs[i].keyword) continue;
		if (!g_strncasecmp(keyword, o_prefs[i].keyword, size)) break;
	}

	if (i >= PT_LAST) return FALSE;

	memset(&pmask, 0, sizeof(struct pmask));

	sk = seek;

	while (*sk && (*sk == ' ' || *sk == '\t')) sk++;

	if (*sk == '[')
	{
		sk++;

		st = next_string(&sk);

		if (*st == '*' && st[1] == 0) pmask.w_type = W_CONSOLE | W_SERVER | W_CHANNEL | W_QUERY | W_DCC;
		else while (*st)
		{
			switch (*st)
			{
				case 'O': pmask.w_type |= W_CONSOLE; break;
				case 'S': pmask.w_type |= W_SERVER; break;
				case 'C': pmask.w_type |= W_CHANNEL; break;
				case 'Q': pmask.w_type |= W_QUERY; break;
				case 'D': pmask.w_type |= W_DCC; break;
				default: return FALSE;
			}
			st++;
		}

		st = next_string(&sk); if (!st) return FALSE;
		if (*st != '*' || st[1] != 0) pmask.network = g_strdup(st);
		st = next_string(&sk); if (!st) return FALSE;
		if (*st != '*' || st[1] != 0) pmask.server = g_strdup(st);
		st = next_string(&sk); if (!st) return FALSE;
		if (*st != '*' || st[1] != 0) pmask.channel = g_strdup(st);
		st = next_string(&sk); if (!st) return FALSE;
		if (*st != '*' || st[1] != 0) pmask.userhost = g_strdup(st);

		while (*sk && (*sk == ' ' || *sk == '\t')) sk++;

		if (*sk++ != ']') return FALSE;

		// printf("Masque trouvé dans les prefs : %d %s %s %s %s\n", pmask.w_type, pmask.network, pmask.server, pmask.channel, pmask.userhost);
	}

	if (!(sk = next_string(&sk))) return FALSE;

	if (o_prefs[i].flags & PREF_GINT) prefs_set_value(i, &pmask, (gpointer) atol(sk));
	else prefs_set_value(i, &pmask, (gpointer) g_strdup(sk));

	return TRUE;
}

void Prefs_Read(FILE *f)
{
	gchar *seek, *keyword;
	gint ksize, line = 0;

	while (fgets(ptmp, sizeof(ptmp), f))
	{
		line++;

		seek = ptmp;

		while (*seek==' ' || *seek=='\t') seek++;
		if (*seek=='\n' || !*seek || *seek=='#') continue;

		keyword = seek;

		while (*seek!=' ' && *seek!='\t' && *seek && *seek!='\n' && *seek!='#') seek++;

		ksize = seek-keyword; if (ksize<=0) continue;

		if (!g_strncasecmp(keyword, "Olirc_Version", ksize)) continue;
		else if (!g_strncasecmp(keyword, "Mask", ksize)) continue;
		else if (!g_strncasecmp(keyword, "UseSocks", ksize)) continue;
		else if (!g_strncasecmp(keyword, "r_window", ksize)) continue;
		else if (!g_strncasecmp(keyword, "Quit_Reason", ksize))
		{
			if (!(keyword = next_string(&seek))) continue;
			if (!*keyword) continue;
			g_free_and_set(GPrefs.Quit_Reason, g_strdup(keyword));
		}
		else if (!g_strncasecmp(keyword, "Open_Server", ksize))
		{
			if (!(keyword = next_string(&seek))) continue;
			Server_Open_List = g_list_append(Server_Open_List, g_strdup(keyword));
		}
		else if (!g_strncasecmp(keyword, "Open_Channel", ksize))
		{
			gchar prefs_tmp[512];
			if (!(keyword = next_string(&seek))) continue;
			sprintf(prefs_tmp, "%s", keyword);
			if (!(keyword = next_string(&seek))) continue;
			if (!*keyword) continue;
			strcat(prefs_tmp, keyword);
			Channel_Open_List = g_list_append(Channel_Open_List, g_strdup(prefs_tmp));
		}
		else if ( (!g_strncasecmp(keyword, "Favorite_Server", ksize))
					|| (!g_strncasecmp(keyword, "Local_Server", ksize)) )
		{
			if (!Prefs_Add_FS(seek)) fprintf(stderr, "Bad Favorite_Server line in prefs file (line %d) -- ignored\n", line);
		}
		else if (!g_strncasecmp(keyword, "Socks", ksize))
		{
			if (!(keyword = next_string(&seek))) continue;
			GPrefs.Socks_Host = g_strdup(keyword);
			if (!(keyword = next_string(&seek))) continue;
			GPrefs.Socks_Port = (atoi(keyword))? atoi(keyword) : 1080;
			if (!(keyword = next_string(&seek))) continue;
			GPrefs.Socks_User = g_strdup(keyword);
		}
/*		else if (!g_strncasecmp(keyword, "UseSocks", ksize))
		{
			if (!(keyword = next_string(&seek))) continue;
			if (!g_strcasecmp("True", keyword)) GPrefs.Flags |= PREF_USE_SOCKS;
		}
*/		else if (!g_strncasecmp(keyword, "Tab_Position", ksize))
		{
			gint i;
			if (!(keyword = next_string(&seek))) continue;
			for (i = 0; i<4; i++)
				if (!g_strcasecmp(GtkPos_Names[i], keyword)) { GPrefs.Tab_Pos = i; break; }
			if (i>=4) fprintf(stderr, "Unknown position name in prefs file (line %d) -- using default\n", line);
		}
		else if (!g_strncasecmp(keyword, "Colors", ksize))
		{
			gint i;
			for (i=0; i<16; i++)
			{
				if (!(keyword = next_string(&seek))) break;
				sscanf(keyword, "0x%6X", &GPrefs.Colors[i]);
			}
		}
		else if (!prefs_read(keyword, ksize, seek))
			fprintf(stderr, "Prefs: Line %d is malformed -- ignored\n", line);
	}
}

void Prefs_Init(gint argc, gchar **argv)
{
	gchar *tmp;
	struct passwd *pwd;
	gchar ptmp[1024];
	struct stat st;
	FILE *prefs;
	gchar *home;

	memset(&GPrefs, 0, sizeof(struct Olirc_Prefs));	/* Erase GPrefs */

	/* Get the preferences directory */

	pwd = getpwuid(getuid());

	if (!pwd) { fprintf(stderr,"\n\nUnable to get your username ?!\n\n"); exit(127); }

	if (pwd->pw_dir) home = pwd->pw_dir;
	else home = getenv("HOME");

	Olirc->username = g_strdup(pwd->pw_name);
	Olirc->home_path = (home && *home)? g_strdup(home) : g_strdup("/tmp");

	tmp = getenv("OLIRC_DIR");

	if (tmp && *tmp)
	{
		if (tmp[strlen(tmp)-1]=='/') sprintf(ptmp,"%s", tmp);
		else sprintf(ptmp,"%s/", tmp);
	}
	else if (home && *home) sprintf(ptmp, "%s/.olirc/", home);
	else
	{
		fprintf(stderr,"\nCan't find your home directory...\n\nPlease define at least the HOME variable !\n\n");
		exit(1);
	}

	Olirc->directory = g_strdup(ptmp);

	/* Create the directory if it doesn't exist yet */

	if (stat(Olirc->directory, &st)==-1)
	{
		if (mkdir(Olirc->directory, 0700)==-1) { fprintf(stderr,"\nmkdir(%s): %s\n\n", Olirc->directory, g_strerror(errno)); exit(1); }
	}
	else if (!(S_ISDIR(st.st_mode))) { fprintf(stderr,"\nFile %s exists but isn't a directory !\n\n", Olirc->directory); exit(1); }

	/* Set default preferences */

	GPrefs.Default_Realname = "Ol-Irc " VER_STRING " by Olrick";
	GPrefs.Default_Quit_Reason = "Ol-Irc " VER_STRING " (" RELEASE_DATE ") by Olrick - " WEBSITE;
	GPrefs.Quit_Reason = g_strdup(GPrefs.Default_Quit_Reason);
	GPrefs.Flags |= PREF_HISTORIES_SEPARATED;
	GPrefs.Tab_Pos = 2;

	{
		gint i;
		guint32 c_def[] = { 0xFFFFFF, 0x000000, 0x000080, 0x009000, 0xFF0000, 0x800000,
								  0xA000A0, 0xFF8000, 0xFFFF00, 0x00FF00, 0x009090, 0x00FFFF,
								  0x0000FF, 0xFF00FF, 0x808080, 0xD0D0D0 };

		for (i=0; i<16; i++) GPrefs.Colors[i] = c_def[i];
	}

	prefs_init();

	global_servers_read();

	/* Try to read preferences file */

	sprintf(ptmp, "%solirc.prefs", Olirc->directory);

	if (stat(ptmp, &st)==-1)
	{
		if (errno!=ENOENT)
		{
			fprintf(stderr,"\nstat(%s): %s\n\n", ptmp, g_strerror(errno));
			exit(1);
		}
	}
	else if (!st.st_size) return;
	
	if ((prefs = fopen(ptmp, "r")))
	{
		/* Parse preferences file */

		Prefs_Read(prefs);
		fclose(prefs);
	}
	else if (errno!=ENOENT) fprintf(stderr,"\nfopen(%s): %s\n\n-- Using default prefs --\n\n", ptmp, g_strerror(errno));
}

void Prefs_Update()
{
	/* Update the preferences file with current user prefs */

	GList *l, *k;
	gchar prefs_path[384];
	gchar prefs_back[384];
	FILE *prefs;
	gint i;
	GUI_Window *rw;

	sprintf(prefs_path, "%solirc.prefs", Olirc->directory);
	sprintf(prefs_back, "%s.backup", prefs_path);

	if (rename(prefs_path, prefs_back)==-1)
	{
		if (errno!=ENOENT)
		{
			fprintf(stderr,"rename(\"%s\",\"%s\"): %s\n", prefs_path, prefs_back, g_strerror(errno));

			if (unlink(prefs_path)==-1)
			{
				fprintf(stderr,"unlink(\"%s\"): %s\n", prefs_path, g_strerror(errno));
				fprintf(stderr,"\n-- Your preferences have not been saved --\n\n");
				return;
			}
		}
	}

	prefs = fopen(prefs_path, "w");
	if (!prefs)
	{
		fprintf(stderr,"fopen(%s, \"w\"): %s\n", prefs_path, g_strerror(errno));
		fprintf(stderr,"\n-- Your preferences have not been saved --\n\n");
		return;
	}
	chmod(prefs_path, 0600);

	fprintf(prefs,"\n# File generated by Ol-Irc " VER_STRING " (" RELEASE_DATE ") on %s\n", irc_date(time(NULL)));

	fprintf(prefs, "\n\n# WARNING : This file is currently being totally reorganized. Do not change anything here now,\n# or Ol-Irc will crash immediately... In the future, you will be able to change what you want.\n\n");

	fprintf(prefs,"\nOlirc_Version " VER_STRING "\n");

	fprintf(prefs,"\n# ---------- Old preferences system ----------\n");

	fprintf(prefs,"\n# Favorite servers chosen by the user\n\n");

	if (Favorite_Servers_List)
	{
		Favorite_Server *fs;
		l = Favorite_Servers_List;
		while (l)
		{
			fs = l->data;
			fprintf(prefs, "Favorite_Server");
			dump_string(prefs, fs->Name, FALSE);
			dump_string(prefs, fs->Ports, FALSE);
			dump_string(prefs, fs->Network, FALSE);
			dump_string(prefs, fs->Location, FALSE);
			dump_string(prefs, fs->Nick, FALSE);
			dump_string(prefs, fs->User, FALSE);
			if (fs->Real && strcmp(fs->Real, GPrefs.Default_Realname)) dump_string(prefs, fs->Real, FALSE);
			else fprintf(prefs, " \"\"");
			fprintf(prefs, " %d", fs->Flags);
			dump_string(prefs, fs->Password, FALSE);
			fprintf(prefs, "\n");
			l = l->next;
		}
	}

	fprintf(prefs,"\n# Servers and channels that will be automatically reopened\n\n");

	l = Servers_List;
	while (l)
	{
		fprintf(prefs, "Open_Server");
		dump_string(prefs, ((Server *) l->data)->fs->Name, FALSE);
		fprintf(prefs, "\n");

		k = ((Server *) l->data)->Channels;
		while (k)
		{
			fprintf(prefs, "Open_Channel");
			dump_string(prefs, ((Server *) l->data)->fs->Name, FALSE);
			dump_string(prefs, ((Channel *) k->data)->Name, FALSE);
			fprintf(prefs, "\n");
			k = k->next;
		}

		l = l->next;
	}

//	fprintf(prefs, "\n# Socks preferences\n\n");
//
//	fprintf(prefs,"Socks");
//	dump_string(prefs, GPrefs.Socks_Host, FALSE);
//	fprintf(prefs, " %d", (GPrefs.Socks_Port)? GPrefs.Socks_Port : 1080);
//	dump_string(prefs, GPrefs.Socks_User, FALSE);
//	fprintf(prefs,"\nUseSocks %s\n", (GPrefs.Flags & PREF_USE_SOCKS)? "True" : "False");

	fprintf(prefs,"\n# Colors preferences\n\n");

	fprintf(prefs, "Colors");
	for (i=0; i<16; i++) fprintf(prefs," 0x%.6X", GPrefs.Colors[i]);
	fprintf(prefs, "\n");

	fprintf(prefs,"\n# Windows preferences\n\n");

	fprintf(prefs,"Tab_Position %s\n", GtkPos_Names[GPrefs.Tab_Pos]);

	fprintf(prefs, "\n");

	if (!is_string_empty(GPrefs.Quit_Reason) && strcmp(GPrefs.Quit_Reason, GPrefs.Default_Quit_Reason))
	{
		fprintf(prefs, "Quit_Reason");
		dump_string(prefs, GPrefs.Quit_Reason, TRUE);
	}
	else
		fprintf(prefs, "Quit_Reason \"\"");

	fprintf(prefs,"\n\n");

	fprintf(prefs, "# -- New preferences system --\n\n");

	l = GW_List;

	while (l)
	{
		gint x, y, width, heigth;
		rw = (GUI_Window *) l->data;
		gdk_window_get_position(rw->Gtk_Window->window, &x, &y);
		gdk_window_get_size(rw->Gtk_Window->window, &width, &heigth);
/*		fprintf(prefs, "r_window %d %d %d %d\n", x, y, width, heigth); */
		l = l->next;
	}

	fprintf(prefs, "\n");

	{
		struct o_sub_pref *osp;

		for (i = 0; i < PT_LAST; i++)
		{	
			l = o_prefs[i].sub_prefs;

			while (l)
			{
				osp = (struct o_sub_pref *) l->data;
				fprintf(prefs, "%s", o_prefs[i].keyword);

				if (osp->pmask.w_type || osp->pmask.network || osp->pmask.server || osp->pmask.channel || osp->pmask.userhost)
				{
					// printf("Sauvegarde du mask %d %s %s %s %s\n", osp->pmask.w_type, osp->pmask.network, osp->pmask.server, osp->pmask.channel, osp->pmask.userhost);
					
					*ptmp = 0;
					
					if ((osp->pmask.w_type & W_ALL) == W_ALL) strcat(ptmp, "*");
					else
					{
						if (osp->pmask.w_type & W_CONSOLE) strcat(ptmp, "O");
						if (osp->pmask.w_type & W_SERVER) strcat(ptmp, "S");
						if (osp->pmask.w_type & W_CHANNEL) strcat(ptmp, "C");
						if (osp->pmask.w_type & W_QUERY) strcat(ptmp, "Q");
						if (osp->pmask.w_type & W_DCC) strcat(ptmp, "D");
					}

					fprintf(prefs, " [ %s", ptmp);

					if (osp->pmask.network) dump_string(prefs, osp->pmask.network, FALSE); else fprintf(prefs, " *");
					if (osp->pmask.server) dump_string(prefs, osp->pmask.server, FALSE); else fprintf(prefs, " *");
					if (osp->pmask.channel) dump_string(prefs, osp->pmask.channel, FALSE); else fprintf(prefs, " *");
					if (osp->pmask.userhost) dump_string(prefs, osp->pmask.userhost, FALSE); else fprintf(prefs, " *");

					fprintf(prefs, " ]");
				}

				if (o_prefs[i].flags & PREF_STRING) dump_string(prefs, (gchar *) osp->value, TRUE);
				else fprintf(prefs, " %d", (gint) osp->value);
				fprintf(prefs, "\n");

				l = l->next;
			}
		}
	}

	fprintf(prefs,"\n\n");
	fclose(prefs);
}

/* vi: set ts=3: */

