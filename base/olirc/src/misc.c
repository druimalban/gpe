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

/*
 * NOTICE - A certain amount of code in this file come from the IRCNet IRC daemon
 *          source files. I have slighty modified some parts of the code to allow
 *          it to compile and run within olirc. If there is any bug, NEVER blame
 *          the original authors of the code, but DO blame ME !
 *
 *         You can find the ircd sources at ftp://ftp.funet.fi/pub/unix/irc/server/
 */

/* TODO Include names and copyrights related to these sources */

#include "olirc.h"
#include "channels.h"
#include "misc.h"

/* <apra/inet.h> seems to prevent any compilation on linux ppc ? TODO Check it */
#undef HAVE_ARPA_INET_H

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include <dirent.h>

gchar misc_tmp[1024];

/* ----- String tables ---------------------------------------------------------------- */

/* These two char tables come from irc2.10.2p1/common/match.c */

guchar tolowertab[] =
{
	0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa,
	0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14,
	0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
	0x1e, 0x1f,
	' ', '!', '"', '#', '$', '%', '&', 0x27, '(', ')',
	'*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	':', ';', '<', '=', '>', '?',
	'@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
	'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
	't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~',
	'_',
	'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
	'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
	't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~',
	0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
	0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
	0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9,
	0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
	0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
	0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
	0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

guchar touppertab[] =
{
	0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa,
	0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14,
	0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
	0x1e, 0x1f,
	' ', '!', '"', '#', '$', '%', '&', 0x27, '(', ')',
	'*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	':', ';', '<', '=', '>', '?',
	'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
	'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
	'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^',
	0x5f,
	'`', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
	'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
	'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^',
	0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
	0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
	0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9,
	0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
	0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
	0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
	0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

/* ----- Strings functions ------------------------------------------------------------ */

gchar *strip_spaces(gchar *str)
{
	gchar *tmp, *r, *w ;
	g_return_val_if_fail(str, NULL);
	r = w = tmp = g_strdup(str);
	while (*r)
	{
		while (*r && *r == ' ') r++;
		while (*r && *r != ' ') *w++ = *r++;
	}
	*w = 0;
	return tmp;
}

gboolean is_string_an_IP(gchar *str)
{
	if (!str) return FALSE;
	while (*str && (*str == '.' || (*str >= '0' && *str <= '9'))) str++;
	return (gboolean) !(*str);
}

gboolean is_string_empty(gchar *str)
{
	if (!str) return TRUE;
	while (*str && (*str == ' ' || *str == '\t')) str++;
	return (gboolean) !(*str);
}

unsigned long convert_IP(gchar *str)
{
	/* convert_IP returns a value in host byte order */

/* Since <inet/arpa.h> can generate compilation errors, we can't use inet_aton() ... TODO Check it */
#undef HAVE_INET_ATON

#ifdef HAVE_INET_ATON
	struct in_addr adr;
	inet_aton(str, &adr);
	return adr.s_addr;
#else
	unsigned long ret = 0;
	gint i;
	for (i=0; i<3; i++)
	{
		ret = (ret << 8) + atoi(str);
		str = strchr(str, '.');
		if (!str) break;
		str++;
	}
	if (str && *str) ret = (ret << 8) + atoi(str);
	return ret;
#endif
}

gint irc_cmp(gchar *s1, gchar *s2)
{
	g_return_val_if_fail(s1, 0);
	g_return_val_if_fail(s2, 0);

	return irc_ncmp(s1, s2, (strlen(s1)>strlen(s2))? strlen(s1) : strlen(s2));
}

gint irc_ncmp(gchar *str1, gchar *str2, guint n)	/* Based on the ircd irc2.10.2p1 sources */
{
	register guchar *s1 = (guchar *) str1;
	register guchar *s2 = (guchar *) str2;
	register gint res;

	if (!n) return 0;
	if (!str1 && !str2) return 0;
	if (!str1) return -1;
	if (!str2) return 1;

	while (!(res = touppertab[(guchar) *s1] - touppertab[(guchar) *s2]))
	{
		s1++; s2++; n--;
		if (!n || (!*s1 && !*s2)) return 0;
	}

	return res;
}

gchar *nick_part(gchar *n)
{
	/* If n is a string like 'foo!bar@some.host.com', return 'foo' */

	register gchar *seek;
	static gchar tmp[256];

	g_return_val_if_fail(n, NULL);

	seek = strcpy(tmp, n);

	while (*seek && *seek != '!') seek++;

	*seek = 0;

	return tmp;
}

gchar *userhost_part(gchar *n)
{
	/* If n is a string like 'foo!bar@some.host.com', return 'bar@some.host.com' */

	register gchar *seek = n;
	static gchar tmp[256];

	g_return_val_if_fail(n, NULL);
	
	while (*seek && *seek != '!') seek++;

	if (*seek) return strcpy(tmp, ++seek);

	return NULL;
}

gchar *network_from_hostname(gchar *host)
{
	/* 'machine.domain.com' gives '*.domain.com'
	   '111.222.33.44' gives '111.222.33.*' */

	static gchar tmp[256];
	gchar *seek = host;
	gboolean IP = TRUE;

	g_return_val_if_fail(host, NULL);

	while (*seek && *seek!='.')
	{
		if (*seek<'0' || *seek>'9') IP = FALSE;
		seek++;
	}

	if (!*seek) { strcpy(tmp, host); return tmp; }

	if (!IP) { sprintf(tmp, "*%s", seek); return tmp; }

	seek++;
	while (*seek && *seek!='.') seek++;
	if (!*seek) { strcpy(tmp, host); return tmp; }
	seek++;
	while (*seek && *seek!='.') seek++;
	if (!*seek) { strcpy(tmp, host); return tmp; }

	memcpy(tmp, host, seek - host + 1);
	tmp[seek-host+1] = '*';
	tmp[seek-host+2] = 0;

	return tmp;
}

gboolean mask_match(gchar *mask, gchar *name)	/* Comes from irc2.10.2p1 sources */
{
	/* Originally written by Douglas A Lewis (dalewis@acsu.buffalo.edu) */

	/* I HAVE MODIFIED THIS CODE : BUGS (if any) ARE MINE */

	/* Warning : Return TRUE if match, FALSE if no match */

	register guchar *m = (guchar *) mask, *n = (guchar *) name;
	gchar	*ma = mask, *na = name;
	gint wild = 0, q = 0, calls = 0;

	while (1)
	{
		if (calls++ > 512) break;

		if (*m == '*')
		{
			while (*m == '*') m++;
			wild = 1;
			ma = (gchar *) m;
			na = (gchar *) n;
		}

		if (!*m)
		{
			if (!*n) return TRUE;
			for (m--; (m > (guchar *) mask) && (*m == '?'); m--)
				;
			if ((m > (guchar *) mask) && (*m == '*') && (m[-1] != '\\')) return TRUE;
			if (!wild) return FALSE;
			m = (guchar *) ma;
			n = (guchar *) ++na;
		}
		else if (!*n)
		{
			while (*m == '*') m++;
			return ((gboolean) !*m);
		}

		if ((*m == '\\') && ((m[1] == '*') || (m[1] == '?')))
		{
			m++;
			q = 1;
		}
		else q = 0;
		
		if ((tolowertab[(guchar) *m] != tolowertab[(guchar) *n]) && ((*m != '?') || q))
		{
			if (!wild) return FALSE;
			m = (guchar *) ma;
			n = (guchar *) ++na;
		}
		else
		{
			if (*m) m++;
			if (*n) n++;
		}
	}

	return FALSE;
}

/* ----- Text functions --------------------------------------------------------------- */

gchar *Strip_Codes(gchar *text, guint mask)
{
	gchar *tmp;

	register gchar *sk = text;
	register gchar *pr = text, *pw;

	g_return_val_if_fail(text, NULL);

	if (!*text) return NULL;

	pw = tmp = (gchar *) g_malloc0(strlen(text) + 2);

	if (mask & CODES_ALL) mask = 0xFFFF;

	do
	{
		while (*sk && *sk!=002 && *sk!=003 && *sk!=026 && *sk!=037 && *sk!=017) sk++;

		memcpy(pw, pr, sk - pr);
		pw += sk - pr;

		if (*sk == 003 && (mask & CODE_COLOR))
		{
			sk++;
			if (*sk && *sk>='0' && *sk<='9')
			{
				sk++; if (*sk>='0' && *sk<='9') sk++;
				if (*sk == ',' && sk[1]>='0' && sk[1]<='9')
				{
					sk += 2; if (*sk>='0' && *sk<='9') sk++;
				}
			}
		}
		else if (*sk == 026 && (mask & CODE_REVERSE)) sk++;
		else if (*sk == 037 && (mask & CODE_UNDERLINE)) sk++;
		else if (*sk == 002 && (mask & CODE_BOLD)) sk++;
		/* XXX Is it really useful to strip TEXT PLAIN codes ??? */
		else if (*sk == 017 && (mask & CODE_PLAIN)) sk++;
		else if (*sk == 007 && (mask & CODE_BEEP)) sk++;
		else if (*sk) *pw++ = *sk++;

		pr = sk;

	} while (*pr);

	return tmp;
}

/* ----- Tokenizer -------------------------------------------------------------------- */

static gchar *token_s = NULL;

gchar *tokenize(gchar *s)
{
	if (!(token_s = s)) return token_s;
	while (*token_s==' ') token_s++;
	if (!*token_s) return (token_s = NULL);
	return token_s;
}

gchar *tok_next(gchar **t, gchar **r)
{
	register gchar *seek = token_s;
	if (!token_s) return (*t = *r = NULL);
	if (!*seek) return (*t = *r = NULL);
	*t = token_s;
	while (*seek && *seek!=' ') seek++;
	if (!*seek) { token_s = seek; return (*r = NULL); }
	*seek++ = 0;
	*r = seek;
	while (*seek==' ') seek++;
	token_s = seek;
	return (*seek)? seek : NULL;
}

/* ----- Time functions --------------------------------------------------------------- */

static gchar irc_date_buffer[64];

gchar *irc_date(time_t t)
{
	strcpy(irc_date_buffer, ctime(&t));
	irc_date_buffer[strlen(irc_date_buffer)-1] = 0;
	return irc_date_buffer;
}

/* ----- Directory functions ---------------------------------------------------------- */

GList *find_files(char *path, char *ext)
{
	GList *l = NULL;
	gchar *e, *r1, *r2;
	DIR *dir;
	struct dirent *ent;
	
	if (!(dir = opendir(path))) return NULL;

	e = ext + strlen(ext);
	
	while ((ent = readdir(dir)))
	{
		r1 = e;
		r2 = ent->d_name + strlen(ent->d_name);

		while (r1 >= ext && *(--r1) == *(--r2));

		if (r1 < ext) l = g_list_append(l, g_strdup(ent->d_name));
	}
	closedir(dir);
	
	return l;
}

/* ----- Misc functions --------------------------------------------------------------- */

gchar *Dump_IP(unsigned long IP)
{
	static gchar ip_buffer[64];
	sprintf(ip_buffer, "%ld.%ld.%ld.%ld", (IP&0xFF000000)>>24, (IP&0xFF0000)>>16, (IP&0xFF00)>>8, (IP&0xFF));
	return ip_buffer;
}

gchar *Remember_Filepath(GtkFileSelection *fs, gchar **store)
{
	gchar *r;
	gchar *file  = gtk_file_selection_get_filename(fs);
	gint t = strlen(file)-1;

	strcpy(misc_tmp, file);
	if (misc_tmp[t] == '/') misc_tmp[t] = 0;
	r = strrchr(misc_tmp, '/'); if (r) *(++r) = 0;

	if (!(*store) || strcmp(*store, file))
	{
		g_free(*store);
		*store = g_strdup(misc_tmp);
	}

	return file;
}

gchar *Duration(unsigned long s)
{
	/* Returns s (seconds) converted into years, weeks, days, hours, minutes, and seconds */

	static gchar d_buf[256];
	gint y = 0, w = 0, d = 0, h = 0, m = 0;

	while (s >= 31536000) { y++; s -= 31536000; }
	while (s >= 604800) { w++; s -= 604800; }
	while (s >= 86400) { d++; s -= 86400; }
	while (s >= 3600) { h++; s -= 3600; }
	while (s >= 60) { m++; s -= 60; }

	*d_buf = 0;

	if (y) { sprintf(misc_tmp, "%d year%s", y, (y>1)? "s" : ""); strspacecat(d_buf, misc_tmp); }
	if (w) { sprintf(misc_tmp, "%d week%s", w, (w>1)? "s" : ""); strspacecat(d_buf, misc_tmp); }
	if (d) { sprintf(misc_tmp, "%d day%s", d, (d>1)? "s" : ""); strspacecat(d_buf, misc_tmp); }
	if (h) { sprintf(misc_tmp, "%d hour%s", h, (h>1)? "s" : ""); strspacecat(d_buf, misc_tmp); }
	if (m) { sprintf(misc_tmp, "%d minute%s", m, (m>1)? "s" : ""); strspacecat(d_buf, misc_tmp); }
	if (s) { sprintf(misc_tmp, "%ld second%s", s, (s>1)? "s" : ""); strspacecat(d_buf, misc_tmp); }

	return d_buf;
}

/* vi: set ts=3: */

