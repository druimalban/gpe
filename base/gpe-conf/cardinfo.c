/*
 * gpe-conf
 *
 * Copyright (C) 2003  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * GPE CF/PC card information and configuration module.
 *
 * Based on Cardinfo by David A. Hinds <dahinds@users.sourceforge.net>
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <libintl.h>
#define _(x) gettext(x)

#include <sys/types.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <poll.h>
#include <time.h>

#include "pcmcia/version.h"
#include "pcmcia/cs_types.h"
#include "pcmcia/cs.h"
#include "pcmcia/cistpl.h"
#include "pcmcia/ds.h"

#include <gtk/gtk.h>

#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>
#include <gpe/popup_menu.h>

#include "cardinfo.h"
#include "applets.h"
#include "suid.h"
#include "parser.h"
#include "misc.h"


/* --- local types and constants --- */

extern gchar *PCMCIA_ERROR;
#define MAX_SOCK 4
#define N_(x) (x)

typedef enum s_state
{
	S_EMPTY, S_PRESENT, S_READY, S_BUSY, S_SUSPEND
}
s_state;

typedef struct event_tag_t
{
	event_t event;
	char *name;
}
event_tag_t;

static event_tag_t event_tag[] = {
	{CS_EVENT_CARD_INSERTION, "card insertion"},
	{CS_EVENT_CARD_REMOVAL, "card removal"},
	{CS_EVENT_RESET_PHYSICAL, "prepare for reset"},
	{CS_EVENT_CARD_RESET, "card reset successful"},
	{CS_EVENT_RESET_COMPLETE, "reset request complete"},
	{CS_EVENT_EJECTION_REQUEST, "user eject request"},
	{CS_EVENT_INSERTION_REQUEST, "user insert request"},
	{CS_EVENT_PM_SUSPEND, "suspend card"},
	{CS_EVENT_PM_RESUME, "resume card"},
	{CS_EVENT_REQUEST_ATTENTION, "request attention"},
};


#define NTAGS (sizeof(event_tag)/sizeof(event_tag_t))

#define MI_CARD_INSERT 	1
#define MI_CARD_EJECT 	2
#define MI_CARD_SUSPEND	3
#define MI_CARD_RESUME	4
#define MI_CARD_RESET	5
#define MI_ASSIGN		6
#define MI_CLOSE		7

static void do_menu (GtkWidget * button, int op);
static void do_driver_dialog (GtkWidget * parent_button);

/*TRANSLATORS: Menu description, '_' are accelerator keys */
static GtkItemFactoryEntry mMain_items[] = {
  { N_("/_Card"),         NULL,         NULL, 0, "<Branch>" },
  { N_("/Card/_Insert"), "", do_menu, MI_CARD_INSERT, "<Item>",(void *) 6},
  { N_("/Card/_Eject"), "", do_menu, MI_CARD_EJECT, "<Item>",(void *) 5},
  { N_("/Card/_Suspend"), "", do_menu, MI_CARD_SUSPEND, "<Item>",(void *) 3},
  { N_("/Card/_Resume"), "", do_menu, MI_CARD_RESUME, "<Item>",(void *) 4},
  { N_("/Card/_Reset"), "", do_menu, MI_CARD_RESET, "<Item>",(void *) 2},
  { N_("/Card/s1"), NULL , NULL,    0, "<Separator>"},
  { N_("/Card/_Assign Driver"), "", do_driver_dialog, MI_ASSIGN, "<StockItem>",GTK_STOCK_JUMP_TO},
  { N_("/Card/s2"), NULL , NULL,    0, "<Separator>"},
  { N_("/Card/_Close"),  NULL, gtk_main_quit, MI_CLOSE, "<StockItem>", GTK_STOCK_QUIT },
  { N_("/_Help"),         NULL,         NULL,           0, "<Branch>" },
  { N_("/_Help/Index"),   NULL,         NULL,    0, "<StockItem>",GTK_STOCK_HELP },
  { N_("/_Help/About"),   NULL,         NULL,    0, "<Item>" },
};

int mMain_items_count = sizeof(mMain_items) / sizeof(GtkItemFactoryEntry);


/* --- module global variables --- */

static GtkWidget *notebook;
static GtkWidget *bookbox;
static GtkWidget *mMain;

static GtkWidget *toolicons[7];

int ns;
static int changed_tab = FALSE;
static int updating = FALSE;
socket_info_t st[MAX_SOCK];
static GList *drivers = NULL;
static const char *pidfile = "/var/run/cardmgr.pid";
const char *pcmcia_tmpcfgfile = "/tmp/config";
const char *pcmcia_cfgfile = "/etc/pcmcia/config";
const char *wlan_ng_cfgfile = "/etc/pcmcia/wlan-ng.conf";
static char *stabfile;

#define PIXMAP_PATH PREFIX "/share/pixmaps/"


/* --- local intelligence --- */


static void
save_config (char *config, int socket)
{
	gchar *content, *tmpval;
	gchar **lines = NULL;
	gint length;
	gchar *delim = NULL;
	FILE *fnew;
	gint i = 0;
	gint j = 0;
	GError *err = NULL;
	char **cfg;
	char *cur_bind;
	int new;
	gboolean use_wlan_ng = FALSE;
	const char *cfgfile = pcmcia_cfgfile;
	gboolean changes = FALSE;

	cfg = g_strsplit (config, "\n", 4);	// idstr,version,manfid,binding
	cur_bind = malloc (strlen (st[socket].card.str) - 5);	// current driver binding
	snprintf (cur_bind, strlen (st[socket].card.str) - 6,
		  st[socket].card.str + 3);
	
	/* determine config file type */
	if (!access(wlan_ng_cfgfile,F_OK) && strstr(config,"prism2_cs"))
	{
		use_wlan_ng = TRUE;
		cfgfile = wlan_ng_cfgfile;
	}
	new = g_str_has_prefix (g_strchomp (cur_bind), _("card not recognized"));	// check if this is a known card

	if (new)
	{
		free (cur_bind);
		cur_bind = cfg[0];	// cur_bind holds the ident line we search for              
	}

	tmpval = "";
	if (!g_file_get_contents (cfgfile, &content, &length, &err))
	{
		fprintf (stderr, "Could not access file: %s.\n",
			 cfgfile);
		if (access (cfgfile, F_OK))	// file exists, but access is denied
		{
			i = 0;
			goto writefile;
		}
	}
	lines = g_strsplit (content, "\n", 4096);
	g_free (content);

	new = TRUE;
	while (lines[i])
	{
		if ((g_strrstr (g_strchomp (lines[i]), g_strchomp (cur_bind)))
		    && (!g_str_has_prefix (g_strchomp (lines[i]), "#")))
		{
			new = FALSE;
			delim = lines[i];
			lines[i] = g_strdup_printf ("%s", cfg[0]);
			free (delim);
			i++;
			if ((config[1]) && strlen (cfg[1]))
			{
				delim = lines[i];
				lines[i] = g_strdup_printf ("%s", cfg[1]);
				free (delim);
				i++;
			}
			if ((config[2]) && strlen (cfg[2]))
			{
				delim = lines[i];
				lines[i] = g_strdup_printf ("%s", cfg[2]);	// really bad if not true
				free (delim);
				i++;
			}
			if ((config[3]) && strlen (cfg[3]))
			{
				delim = lines[i];
				lines[i] = g_strdup_printf ("%s", cfg[3]);
				free (delim);
				i++;
			}
			if ((lines[i]) && (!strstr (lines[i], "card \"")))
			{
				free (lines[i]);
				lines[i] = g_strdup ("");
			}

		}
		i++;
	}

	i--;


/* writes info to configfile */	
writefile:

	if (new)
	{
		lines = realloc (lines, (i + 6) * sizeof (gchar *));
		for (j=i;j<i+6;j++)
			lines[j] = NULL;
		lines[i] = g_strdup ("");
		i++;
		lines[i] = g_strdup_printf ("%s", cfg[0]);
		i++;
		if ((config[1]) && strlen (cfg[1]))
		{
			lines[i] = g_strdup_printf ("%s", cfg[1]);
			i++;
		}
		if ((config[2]) && strlen (cfg[2]))
		{
			lines[i] = g_strdup_printf ("%s", cfg[2]);
			i++;
		}
		if ((config[3]) && strlen (cfg[3]))
		{
			lines[i] = g_strdup_printf ("%s", cfg[3]);
			i++;
		}
		lines[i] = g_strdup ("");
		i++;
		lines[i] = NULL;
	}

	fnew = fopen (pcmcia_tmpcfgfile, "w");
	if (!fnew)
	{
		gpe_error_box (_("Could not write to temporal config file."));
		return;
	}

	for (j = 0; j < i; j++)
	{
		fprintf (fnew, "%s\n", lines[j]);
	}
	fclose (fnew);
	g_strfreev (lines);

	/* copy new file */
	if (use_wlan_ng)
		suid_exec ("CMCP", "1");
	else
		suid_exec ("CMCP", "0");
	usleep(400000);

	/* do we have wlan-ng at all? */
	if (!access(wlan_ng_cfgfile,F_OK)) 
	{
		/* search other files, eliminate all entries for same id */
		if (use_wlan_ng)
			cfgfile = pcmcia_cfgfile;
		else
			cfgfile = wlan_ng_cfgfile;
		
		if (!g_file_get_contents (cfgfile, &content, &length, &err))
		{
			fprintf (stderr, "Could not access file: %s.\n",
				 cfgfile);
		}
		lines = g_strsplit (content, "\n", 4096);
		g_free (content);
		
		/* clean strings for easier identification */
		for (i=0;i<4;i++)
		{
			delim = cfg[i];
			cfg[i]=g_strchomp(cfg[i]);
		}
		
		i = 1;
		while (lines[i])
		{
			if (g_str_has_prefix (g_strchomp (lines[i]), "card") && lines[i+1] && lines[i+2] &&
				(strstr(lines[i+1],cfg[1]) || strstr(lines[i+2],cfg[1]) || strstr(lines[i+1],cfg[2]) || strstr(lines[i+2],cfg[2])))
			{
				changes = TRUE;
				for (j=0;j<3;j++)
				{
					delim = lines[i+j];
					lines[i+j] = g_strdup_printf ("# %s", delim);
					free (delim);
				}
				i+=2;
				if (lines[i+1] && strlen(lines[i+1]))
				{
					delim = lines[i+1];
					lines[i+1] = g_strdup_printf ("# %s", delim);
					free (delim);
				}
			}
			i++;
		}
		
		if (changes)
		{
			fnew = fopen (pcmcia_tmpcfgfile, "w");
			if (!fnew)
			{
				gpe_error_box (_("Could not write to temporal config file."));
				return;
			}
	
			for (j = 0; j < i; j++)
			{
				fprintf (fnew, "%s\n", lines[j]);
			}
			fclose (fnew);
			
			/* copy new file */
			if (use_wlan_ng)
				suid_exec ("CMCP", "0");
			else
				suid_exec ("CMCP", "1");
			usleep(400000);
		}
		g_strfreev (lines);
	}
}  


static char *
user_get_card_ident (int socket)
{
	static char str[256];
	struct pollfd pfd[1];
	int i = 0, j = 0;
	char *result = 0;

	if (setvbuf (suidin, NULL, _IONBF, 0) != 0)
		fprintf (stderr, "gpe-conf: error setting buffer size!");

	sprintf (str, "%d", socket);
	suid_exec ("CMID", str);

	pfd[0].fd = suidinfd;
	pfd[0].events = (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI);

	/* we hope and pray */
	while ((i < 4) && (j < 20))
	{
		j++;
		while (poll (pfd, 1, 500))
		{
			fgets (str, 255, suidin);
			i++;
			if (i == 1)
				result = g_strdup (str);
			else
			{
				result = realloc (result,
						  strlen (result) +
						  strlen (str) + 1);
				strcat (result, str);
			}
		}
	}
	return result;
}


static char *
get_card_ident (int socket)
{
	FILE *pipe;
	char buffer[256];
	char *result = NULL;

	sprintf (buffer, "/sbin/cardctl info %d", socket);
	pipe = popen (buffer, "r");

	if (pipe > 0)
	{
		while ((feof (pipe) == 0))
		{
			fgets (buffer, 255, pipe);
			if (strstr (buffer, "PRODID_1=\""))
			{
				result = malloc (strlen (buffer) - 10);
				snprintf (result, strlen (buffer) - 11, "%s ",
					  buffer + 10);
				strncat (result, " ", 1);
			}
			if (strstr (buffer, "PRODID_2=\""))
			{
				result = realloc (result,
						  strlen (result) +
						  strlen (buffer) - 9);
				strncat (result, buffer + 10,
					 strlen (buffer) - 12);
				strncat (result, "\n", 1);
			}
/*			if (strstr(buffer,"PRODID_3=\""))
			{
				result = realloc(result,strlen(result)+strlen(buffer)-9);
				strncat(result,buffer+10,strlen(buffer)-12);
				strncat(result," ",1);
			}
			if (strstr(buffer,"PRODID_4=\""))
			{
				result = realloc(result,strlen(result)+strlen(buffer)-9);
				strncat(result,buffer+10,strlen(buffer)-12);
				strncat(result,"\n",1);
			}
*/ if (strstr (buffer, "MANFID="))
			{
				result = realloc (result,
						  strlen (result) +
						  strlen (buffer));
				strncat (result, "\n0x", 3);
				strncat (result, buffer + 7, 5);
				strncat (result, " 0x", 3);
				strncat (result, buffer + 12, 4);
				strncat (result, "\n", 1);
			}
			if (strstr (buffer, "FUNCID="))
			{
				result = realloc (result,
						  strlen (result) +
						  strlen (buffer) - 6);
				strncat (result, buffer + 7,
					 strlen (buffer) - 8);
				return result;
			}
		}
		pclose (pipe);
		return result;
	}
	return NULL;
}


void
do_get_card_ident (int socket)
{
	char *id = NULL;

	id = get_card_ident (socket);
	if (setvbuf (nsreturn, NULL, _IONBF, 0) != 0)
		fprintf (stderr, "gpe-conf: error setting buffer size!");
	fprintf (nsreturn, "%s\n", id);
	fflush (nsreturn);
	fsync (nsreturnfd);
	free (id);
}



static int
get_driver_list ()
{
	FILE *pipe;
	char buffer[256], dr[128];

	pipe = popen
		("/bin/grep device /etc/pcmcia/config | /bin/grep -v \"#\"",
		 "r");

	if (pipe > 0)
	{
		while ((feof (pipe) == 0))
		{
			fgets (buffer, 255, pipe);
			if (sscanf (buffer, "device \"%s\"", dr))
			{
				dr[strcspn (dr, "\"")] = '\0';
				drivers =
					g_list_append (drivers,
						       g_strdup (dr));
			}
		}
		pclose (pipe);
		
		if (!access(wlan_ng_cfgfile,F_OK))
			drivers =
				g_list_append (drivers, g_strdup ("prism2_cs"));
		
		return 0;
	}
	return -1;
}


static void
do_alert (char *fmt, ...)
{
	char msg[132];
	va_list args;
	va_start (args, fmt);
	vsprintf (msg, fmt, args);
	gpe_error_box (msg);
	va_end (args);
}				/* do_alert */


void
do_reset ()
{
	FILE *f;
	pid_t pid;

	/* this seems not to work in familiar :-( */
	f = fopen (pidfile, "r");
	if (f == NULL)
	{
		do_alert ("%s %s", _("Could not open pidfile:"),
			  strerror (errno));
		return;
	}
	if (fscanf (f, "%d", &pid) != 1)
	{
		do_alert (_("Could not read pidfile"));
		return;
	}
	if (kill (pid, SIGHUP) != 0)
		do_alert ("%s %s", _("Could not signal cardmgr:"),
			  strerror (errno));

	/* ... this should work everytime */

	system ("/etc/init.d/pcmcia restart");

}


static void
reset_cardmgr (GtkWidget * button)
{
	suid_exec ("CMRE", "CMRE");
}


static int
lookup_dev (char *name)
{
	FILE *f;
	int n;
	char s[32], t[32];

	f = fopen ("/proc/devices", "r");
	if (f == NULL)
		return -errno;
	while (fgets (s, 32, f) != NULL)
	{
		if (sscanf (s, "%d %s", &n, t) == 2)
			if (strcmp (name, t) == 0)
				break;
	}
	fclose (f);
	if (strcmp (name, t) == 0)
		return n;
	else
		return -ENODEV;
}				/* lookup_dev */


static int
open_dev (dev_t dev)
{
	char *fn;
	int fd;

	if ((fn = tmpnam (NULL)) == NULL)
		return -1;
	if (mknod (fn, (S_IFCHR | S_IREAD), dev) != 0)
		return -1;
	if ((fd = open (fn, O_RDONLY)) < 0)
	{
		unlink (fn);
		return -1;
	}
	if (unlink (fn) != 0)
	{
		close (fd);
		return -1;
	}
	return fd;
}				/* open_dev */


int
init_pcmcia_suid ()
{
	int major;
	servinfo_t serv;

	if (geteuid () != 0)
	{
		PCMCIA_ERROR =
			g_strdup (_
				  ("gpe-conf must be setuid root to use cardinfo feature."));
		return -1;
	}

	if (access ("/var/lib/pcmcia", R_OK) == 0)
	{
		stabfile = "/var/lib/pcmcia/stab";
	}
	else
	{
		stabfile = "/var/run/stab";
	}

	major = lookup_dev ("pcmcia");
	if (major < 0)
	{
		if (major == -ENODEV)
			PCMCIA_ERROR =
				g_strdup (_
					  ("No PCMCIA driver in '/proc/devices'"));
		else
			PCMCIA_ERROR =
				g_strdup (_("Could not open '/proc/devices'"));
		return -1;
	}

	for (ns = 0; ns < MAX_SOCK; ns++)
	{
		st[ns].fd = open_dev ((major << 8) + ns);
		if (st[ns].fd < 0)
			break;
	}
	if (ns == 0)
	{
		PCMCIA_ERROR = g_strdup (_("No sockets found\n"));
		return -1;
	}

	if (ioctl (st[0].fd, DS_GET_CARD_SERVICES_INFO, &serv) == 0)
	{
		if (serv.Revision != CS_RELEASE_CODE)
			PCMCIA_ERROR =
				g_strdup (_
					  ("Card Services release does not match!"));
	}
	else
	{
		PCMCIA_ERROR =
			g_strdup (_("Could not get CS revision info!\n"));
		return -1;
	}
	return 0;
}


GtkWidget *
new_field (field_t * field, GtkWidget * parent, int pos, char *strlabel)
{
	GtkWidget *result;
	GtkWidget *label;

	label = gtk_label_new (strlabel);
	result = gtk_label_new (NULL);
	gtk_table_attach (GTK_TABLE (parent), label, 0, 1, pos, pos + 1,
			  GTK_FILL, 0, 0, 0);
	gtk_table_attach (GTK_TABLE (parent), result, 1, 2, pos, pos + 1,
			  GTK_SHRINK | GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_SHRINK | GTK_EXPAND, 0, 0);
	field->widget = result;
	gtk_misc_set_alignment (GTK_MISC (result), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

	field->str = strdup ("");

	return result;
}

static void
new_flag (flag_t * flag, GtkWidget * parent, char *strlabel)
{
	char *str;
	str = g_strdup_printf (" %s ", strlabel);
	flag->widget = gtk_label_new (str);
	free (str);
	gtk_misc_set_alignment (GTK_MISC (flag->widget), 0.5, 0.5);
	gtk_box_pack_start (GTK_BOX (parent), flag->widget, FALSE, TRUE, 0);
	gtk_widget_set_sensitive (flag->widget, FALSE);
	flag->val = 0;
}

static void
update_field (field_t * field, char *new)
{
	if (strcmp (field->str, new) != 0)
	{
		free (field->str);
		field->str = strdup (new);
		gtk_label_set_markup (GTK_LABEL (field->widget), new);
	}
}

static void
update_flag (flag_t * flag, int new)
{
	char tmp[80];
	if (flag->val != new)
	{
		flag->val = new;
		if (new)
			snprintf (tmp, 80,
				  "<span background=\"#000000\" foreground=\"#DDDDDD\">%s</span>",
				  gtk_label_get_text (GTK_LABEL
						      (flag->widget)));
		else
			snprintf (tmp, 80, "%s",
				  gtk_label_get_text (GTK_LABEL
						      (flag->widget)));
		gtk_label_set_markup (GTK_LABEL (flag->widget), tmp);
		gtk_widget_set_sensitive (flag->widget, new);
	}
}

static void
update_icon (char *type, GtkWidget * container)
{
	GtkWidget *ctype_pixmap;
	char *fname;

	/* transfer types we know */
	if (!strcmp(type,"254"))
		sprintf(type,"usb");
	if (!strcmp(type,"wlan-ng"))
		sprintf(type,"network");
	
	fname = g_strdup_printf ("%spccard-%s.png", PIXMAP_PATH, type);
	if (access (fname, R_OK))
	{
		free (fname);
		fname = g_strdup_printf ("%spccard-unknown.png", PIXMAP_PATH);
	}
	ctype_pixmap = gtk_bin_get_child (GTK_BIN (container));
	if (ctype_pixmap)
	{
		gtk_container_remove (GTK_CONTAINER (container),
				      ctype_pixmap);
	}
	ctype_pixmap = gpe_create_pixmap (container, fname, 48, 48);
	gtk_container_add (GTK_CONTAINER (container), ctype_pixmap);
	gtk_widget_show (GTK_WIDGET (ctype_pixmap));
	free (fname);
}

static int
do_update (GtkWidget * widget)
{
	FILE *f;
	int i, j, event, ret, state;
	cs_status_t status;
	config_info_t cfg;
	char s[80], *t, d[80], type[20], drv[20], io[20], irq[4];
	ioaddr_t stop;
	struct stat buf;
	static time_t last = 0;
	time_t now;
	struct tm *tm;
	fd_set fds;
	struct timeval timeout;

	if (updating) return TRUE;
	updating = TRUE;

	/* Poll for events */
	FD_ZERO (&fds);
	for (i = 0; i < ns; i++)
		FD_SET (st[i].fd, &fds);
	timeout.tv_sec = timeout.tv_usec = 0;
	ret = select (MAX_SOCK + 4, &fds, NULL, NULL, &timeout);
	now = time (NULL);
	tm = localtime (&now);
	if (ret > 0)
	{
		for (i = 0; i < ns; i++)
		{
			if (!FD_ISSET (st[i].fd, &fds))
				continue;
			ret = read (st[i].fd, &event, 4);
			if (ret != 4)
				continue;
			for (j = 0; j < NTAGS; j++)
				if (event_tag[j].event == event)
					break;
			if (j == NTAGS)
				sprintf (s,
					 "%2d:%02d:%02d  socket %d: unknown event 0x%x",
					 tm->tm_hour, tm->tm_min, tm->tm_sec,
					 i, event);
			else
				sprintf (s, "%2d:%02d:%02d  socket %d: %s",
					 tm->tm_hour, tm->tm_min, tm->tm_sec,
					 i, event_tag[j].name);
//          fl_addto_browser(event_log, s);
		}
	}

	if ((stat (stabfile, &buf) == 0) && (buf.st_mtime >= last))
	{
		f = fopen (stabfile, "r");
		if (f == NULL)
			return TRUE;

		if (flock (fileno (f), LOCK_SH) != 0)
		{
			do_alert ("%s: %s", _("flock(stabfile) failed"),
				  strerror (errno));
			return FALSE;
		}
		last = now;
		fgetc (f);
		for (i = 0; i < ns; i++)
		{
			if (!fgets (s, 80, f))
				break;
			s[strlen (s) - 1] = '\0';
			/* if you change formating here, change <b>empty</b> too!*/
			if (strstr(s,"unsupported card"))		
				snprintf (d, 80, "<b>%s</b>", _("card not recognized"));
			else
				snprintf (d, 80, "<b>%s</b>", s + 9);
			update_field (&st[i].card, d);
			*d = '\0';
			*type = '\0';
			snprintf(drv,20,_("no driver loaded"));
			for (;;)
			{
				int c = fgetc (f);
				if ((c == EOF) || (c == 'S'))
				{
					update_field (&st[i].dev, d);
					update_field (&st[i].type, type);
					update_icon (type,
						     st[i].statusbutton);
					update_field (&st[i].driver, drv);
					break;
				}
				else
				{
					fgets (s, 80, f);
					t = s;
					t = strchr (t, '\t') + 1;
					snprintf (type,
						  strcspn (t, "\t\n") + 1,
						  "%s", t);
					t = strchr (t, '\t') + 1;
					snprintf (drv,
						  strcspn (t, "\t\n") + 1,
						  "%s", t);
					for (j = 0; j < 2; j++)
						t = strchr (t, '\t') + 1;
					t[strcspn (t, "\t\n")] = '\0';
					if (*d == '\0')
						strcpy (d, t);
					else
					{
						strcat (d, ", ");
						strcat (d, t);
					}
				}
			}
		}
		flock (fileno (f), LOCK_UN);
		fclose (f);
	}

	for (i = 0; i < ns; i++)
	{

		state = S_EMPTY;
		status.Function = 0;
		ioctl (st[i].fd, DS_GET_STATUS, &status);
		if (strcmp (st[i].card.str, "<b>empty</b>") == 0)
		{
			if (status.CardState & CS_EVENT_CARD_DETECT)
				state = S_PRESENT;
		}
		else
		{
			if (status.CardState & CS_EVENT_PM_SUSPEND)
				state = S_SUSPEND;
			else
			{
				if (status.CardState & CS_EVENT_READY_CHANGE)
					state = S_READY;
				else
					state = S_BUSY;
			}
		}

		if ((state != st[i].o_state) || changed_tab)
		{
			st[i].o_state = state;
			if (i == (changed_tab - 1))
				changed_tab = 0;
			if (i ==
			    gtk_notebook_get_current_page (GTK_NOTEBOOK
							   (notebook)))
			{
				for (j = 1; j <= 6; j++)
					gtk_widget_set_sensitive (toolicons
								  [j], FALSE);
			}
			j = gtk_notebook_get_current_page (GTK_NOTEBOOK
							   (notebook));
			switch (state)
			{
			case S_EMPTY:
				update_field (&st[i].state, "");
				gtk_widget_set_sensitive (toolicons
							  [6], TRUE);
				break;
			case S_PRESENT:
				if (j == i)
					gtk_widget_set_sensitive (toolicons
								  [5], TRUE);
				update_field (&st[i].state, "");
				break;
			case S_READY:
				if (j == i)
					gtk_widget_set_sensitive (toolicons
								  [1], TRUE);
				if (j == i)
					gtk_widget_set_sensitive (toolicons
								  [2], TRUE);
				if (j == i)
					gtk_widget_set_sensitive (toolicons
								  [3], TRUE);
				if (j == i)
					gtk_widget_set_sensitive (toolicons
								  [5], TRUE);
				snprintf (d, 80,
					  "<span foreground=\"#00A000\">%s</span>",
					  _("ready"));
				update_field (&st[i].state, d);
				break;
			case S_BUSY:
				if (j == i)
					gtk_widget_set_sensitive (toolicons
								  [1], TRUE);
				if (j == i)
					gtk_widget_set_sensitive (toolicons
								  [2], TRUE);
				if (j == i)
					gtk_widget_set_sensitive (toolicons
								  [3], TRUE);
				if (j == i)
					gtk_widget_set_sensitive (toolicons
								  [5], TRUE);
				snprintf (d, 80,
					  "<span foreground=\"#B00000\">%s</span>",
					  _("not ready"));
				update_field (&st[i].state, d);
				break;
			case S_SUSPEND:
				if (j == i)
					gtk_widget_set_sensitive (toolicons
								  [1], TRUE);
				if (j == i)
					gtk_widget_set_sensitive (toolicons
								  [4], TRUE);
				if (j == i)
					gtk_widget_set_sensitive (toolicons
								  [5], TRUE);
				snprintf (d, 80,
					  "<span foreground=\"#000090\">%s</span>",
					  _("suspended"));
				update_field (&st[i].state, d);
				break;
			}
		}

		strcpy (io, "");
		strcpy (irq, "");
		memset (&cfg, 0, sizeof (cfg));
		ret = ioctl (st[i].fd, DS_GET_CONFIGURATION_INFO, &cfg);
		if (cfg.Attributes & CONF_VALID_CLIENT)
		{
			if (cfg.AssignedIRQ != 0)
				sprintf (irq, "%d", cfg.AssignedIRQ);
			if (cfg.NumPorts1 > 0)
			{
				stop = cfg.BasePort1 + cfg.NumPorts1;
				if (cfg.NumPorts2 > 0)
				{
					if (stop == cfg.BasePort2)
						sprintf (io, "%#x-%#x",
							 cfg.BasePort1,
							 stop +
							 cfg.NumPorts2 - 1);
					else
						sprintf (io,
							 "%#x-%#x, %#x-%#x",
							 cfg.BasePort1,
							 stop - 1,
							 cfg.BasePort2,
							 cfg.BasePort2 +
							 cfg.NumPorts2 - 1);
				}
				else
					sprintf (io, "%#x-%#x", cfg.BasePort1,
						 stop - 1);
			}
		}
		update_field (&st[i].irq, irq);
		update_field (&st[i].io, io);

		update_flag (&st[i].cd,
			     status.CardState & CS_EVENT_CARD_DETECT);
		update_flag (&st[i].vcc, cfg.Vcc > 0);
		update_flag (&st[i].vpp, cfg.Vpp1 > 0);
		update_flag (&st[i].wp,
			     status.CardState & CS_EVENT_WRITE_PROTECT);
	}
	updating = FALSE;
	return TRUE;
}


void
do_ioctl (int chan, int cmd)
{
	int ret = 0;
	switch (cmd)
	{
	case CMD_RESET:
		ret = ioctl (st[chan].fd, DS_RESET_CARD);
		break;
	case CMD_SUSPEND:
		ret = ioctl (st[chan].fd, DS_SUSPEND_CARD);
		break;
	case CMD_RESUME:
		ret = ioctl (st[chan].fd, DS_RESUME_CARD);
		break;
	case CMD_EJECT:
		ret = ioctl (st[chan].fd, DS_EJECT_CARD);
		break;
	case CMD_INSERT:
		ret = ioctl (st[chan].fd, DS_INSERT_CARD);
		break;
	}
	if (ret != 0)
		fprintf (stderr, "%s", strerror (errno));
}


static void
do_menu (GtkWidget * button, int op)
{
	int i = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));
	char *cmdstr = NULL;

	switch (op)
	{
	case 1:
		break;
	case 2:
		cmdstr = g_strdup_printf ("%d %d", i, CMD_RESET);
		break;
	case 3:
		cmdstr = g_strdup_printf ("%d %d", i, CMD_SUSPEND);
		break;
	case 4:
		cmdstr = g_strdup_printf ("%d %d", i, CMD_RESUME);
		break;
	case 5:
		cmdstr = g_strdup_printf ("%d %d", i, CMD_EJECT);
		break;
	case 6:
		cmdstr = g_strdup_printf ("%d %d", i, CMD_INSERT);
		break;
	}
	suid_exec ("CMCO", cmdstr);
	free (cmdstr);
}				/* do_menu */


static void
do_tabchange (GtkNotebook * notebook,
	      GtkNotebookPage * page, guint page_num, gpointer user_data)
{
	changed_tab = page_num + 1;
	do_update(NULL);
}


void
dialog_driver_response (GtkDialog * dialog, gint response, gpointer param)
{
	char *ident, *config, **buf, *tmp;
	GtkWidget *eDriver;

	if (response == GTK_RESPONSE_ACCEPT)
	{
		eDriver = gtk_object_get_data (GTK_OBJECT (dialog), "driver");
		ident = gtk_object_get_data (GTK_OBJECT (dialog), "ident");

		buf = g_strsplit (ident, "\n", 4);
		if (buf)
		{
			config = g_strdup_printf ("card \"%s\"\n", buf[0]);
			if (strlen (buf[1]) > 2)
			{
				tmp = g_strdup_printf ("%s  version \"%s\"\n",
						       config, buf[1]);
				free (config);
				config = tmp;
			}
			if (strlen (buf[2]))	// should be true in all cases
			{
				tmp = g_strdup_printf ("%s  manfid %s\n",
						       config, buf[2]);
				free (config);
				config = tmp;
			}
			g_strfreev (buf);
			free (ident);
			ident = strdup (gtk_entry_get_text
					(GTK_ENTRY (eDriver)));

			tmp = g_strdup_printf ("%s  bind \"%s\"\n", config,
					       ident);
			free (config);
			free (ident);
			config = tmp;

			/* save new config */
			save_config (config, (int) param);
		
			free (config);
			reset_cardmgr (NULL);
		}
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));
}


static void
do_driver_dialog (GtkWidget * parent_button)
{
	GtkWidget *w, *cb, *l;
	int i;
	char *identmsg = NULL;
	char **idents;
	char *str;
    static char *fn[] = {
	"multifunction", "memory", "serial", "parallel",
	"fixed disk", "video", "network", "AIMS", "SCSI"
    };

	i = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));
	cb = gtk_combo_new ();
	gtk_widget_show (cb);
	gtk_combo_set_popdown_strings (GTK_COMBO (cb), drivers);
	gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (cb)->entry),
			    st[i].driver.str);

	w = gtk_dialog_new_with_buttons (_("Assign Driver"),
					 GTK_WINDOW (mainw),
					 GTK_DIALOG_MODAL |
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_STOCK_CANCEL,
					 GTK_RESPONSE_REJECT, GTK_STOCK_OK,
					 GTK_RESPONSE_ACCEPT, NULL);

	l = gtk_label_new (_
			   ("This dialog allows you to select/\nchange the driver "\
			   "assignment for a\nPCMCIA or CF card."));
	gtk_misc_set_alignment (GTK_MISC (l), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (w)->vbox), l, FALSE, TRUE,
			    gpe_get_boxspacing ());
	gtk_widget_show (l);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (w)->vbox), cb, FALSE, TRUE,
			    gpe_get_boxspacing ());

	l = gtk_label_new (_("Your current card identifies itself as:"));
	gtk_misc_set_alignment (GTK_MISC (l), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (w)->vbox), l, FALSE, TRUE,
			    gpe_get_boxspacing ());
	gtk_widget_show (l);

	identmsg = user_get_card_ident (i);
	idents = g_strsplit (identmsg, "\n", 4);
	l = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (l), 0.0, 0.5);
	
	if ((atoi(idents[3]) >= 0) && (atoi(idents[3]) <= 7))
	{
		str = g_strdup_printf ("<i>%s\n%s: %s\n%s: %s (%s)</i>", idents[0],
			       _("Manuf. Id"), idents[2], _("Class"),
			       fn[atoi(idents[3])],g_strstrip(idents[3]));
	}
	else
	{
		str = g_strdup_printf ("<i>%s\n%s: %s\n%s: %s</i>", idents[0],
			       _("Manuf. Id"), idents[2], _("Class"),
			       idents[3]);
	}
	gtk_label_set_markup (GTK_LABEL (l), str);
	free (str);
	g_strfreev (idents);
	gtk_widget_show (l);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (w)->vbox), l, FALSE, TRUE,
			    gpe_get_boxspacing ());

	gtk_object_set_data (GTK_OBJECT (w), "ident", identmsg);
	gtk_object_set_data (GTK_OBJECT (w), "driver", GTK_COMBO (cb)->entry);

	/* connect response, submit socket as param */
	g_signal_connect (GTK_OBJECT (w), "response",
			  (void *) dialog_driver_response, (void *) i);

	gtk_dialog_run (GTK_DIALOG (w));
}


/* create menus from description */
GtkWidget *
create_mMain(GtkWidget  *window)
{
	GtkItemFactory *itemfactory;
	GtkAccelGroup *accelgroup;

	accelgroup = gtk_accel_group_new ();

	itemfactory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>",
                                       accelgroup);
	gtk_item_factory_create_items (itemfactory, mMain_items_count, 
		mMain_items, NULL);
	gtk_window_add_accel_group (GTK_WINDOW (window), accelgroup);

	toolicons[1] = gtk_item_factory_get_item_by_action(itemfactory,MI_ASSIGN);
	toolicons[2] = gtk_item_factory_get_item_by_action(itemfactory,MI_CARD_RESET);
	toolicons[3] = gtk_item_factory_get_item_by_action(itemfactory,MI_CARD_SUSPEND);
	toolicons[4] = gtk_item_factory_get_item_by_action(itemfactory,MI_CARD_RESUME);
	toolicons[5] = gtk_item_factory_get_item_by_action(itemfactory,MI_CARD_EJECT);
	toolicons[6] = gtk_item_factory_get_item_by_action(itemfactory,MI_CARD_INSERT);
	
	return (gtk_item_factory_get_widget (itemfactory, "<main>"));
}


/* --- gpe-conf interface --- */

void
Cardinfo_Free_Objects ()
{
	g_list_free (drivers);
}

void
Cardinfo_Save ()
{

}

void
Cardinfo_Restore ()
{

}

GtkWidget *
Cardinfo_Build_Objects (void)
{
	GtkWidget *label, *ctype_pixmap;
	GtkWidget *table, *hbox;
	gchar iname[100];
	GtkTooltips *tooltips;
	int i;
	gchar *errmsg;

	tooltips = gtk_tooltips_new ();

	bookbox = gtk_vbox_new (FALSE, gpe_get_boxspacing ());
	gtk_container_set_border_width (GTK_CONTAINER (bookbox), 0);

	/* main menu */ 
	mMain = create_mMain(mainw);
	gtk_box_pack_start(GTK_BOX(bookbox),mMain,FALSE,TRUE,0);
	
	notebook = gtk_notebook_new ();
	gtk_container_set_border_width (GTK_CONTAINER (notebook), 0);
	gtk_object_set_data (GTK_OBJECT (notebook), "tooltips", tooltips);

	/* toolbar and packing */
	gtk_box_pack_start (GTK_BOX (bookbox), notebook, TRUE, TRUE, 0);

	/* socket tabs */
	for (i = 0; i < ns; i++)
	{
		table = gtk_table_new (3, 8, FALSE);
		gtk_tooltips_set_tip (tooltips, table,
				      _("PC/CF card socket information."),
				      NULL);
		gtk_container_set_border_width (GTK_CONTAINER (table),
						gpe_get_border ());
		gtk_table_set_row_spacings (GTK_TABLE (table), 0);
		gtk_table_set_col_spacings (GTK_TABLE (table),
					    gpe_get_boxspacing ());

		sprintf (iname, "%s %d", _("Socket"), i);
		label = gtk_label_new (iname);
		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table,
					  label);
		gtk_tooltips_set_tip (tooltips, label,
				      _("PC/CF card socket information."),
				      NULL);

		/* box for header */
		hbox = gtk_hbox_new (FALSE, gpe_get_boxspacing ());
		gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
		gtk_table_attach (GTK_TABLE (table), hbox, 0, 3, 0, 1,
				  GTK_FILL, 0, 0, 0);

		/* graphic button */
		label = gtk_button_new ();
		st[i].statusbutton = label;
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
		gtk_button_set_relief (GTK_BUTTON (label), GTK_RELIEF_NONE);

		ctype_pixmap =
			gpe_create_pixmap (label,
					   PIXMAP_PATH "pccard-unknown.png",
					   48, 48);
		gtk_container_add (GTK_CONTAINER (label), ctype_pixmap);

		/* top label */
		label = gtk_label_new (NULL);
		gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
		gtk_widget_set_size_request (label, 160, 40);
		snprintf (iname, 100, "<b>%s %d</b>",
			  _("Information for socket"), i);
		gtk_label_set_markup (GTK_LABEL (label), iname);
		gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

		st[i].card.widget = label;
		st[i].card.str = strdup ("");

		/* fields */
		new_field (&st[i].state, table, 2, _("State:"));
		new_field (&st[i].dev, table, 3, _("Device:"));
		new_field (&st[i].driver, table, 4, _("Driver:"));
		new_field (&st[i].type, table, 5, _("Type:"));
		new_field (&st[i].io, table, 6, _("I/O:"));
		new_field (&st[i].irq, table, 7, _("IRQ:"));

		/* flags */
		hbox = gtk_hbox_new (TRUE, gpe_get_boxspacing ());
		gtk_table_attach (GTK_TABLE (table), hbox, 0, 4, 8, 9,
				  GTK_FILL, 0, 0, 2);

		label = gtk_label_new (_("Flags:"));
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);

		new_flag (&st[i].cd, hbox, "CD");
		new_flag (&st[i].vcc, hbox, "VCC");
		new_flag (&st[i].vpp, hbox, "VPP");
		new_flag (&st[i].wp, hbox, "WP");

	}

	g_signal_connect_after (G_OBJECT (notebook), "switch-page",
				G_CALLBACK (do_tabchange), NULL);
	
	if (PCMCIA_ERROR)
	{
		errmsg = g_strdup_printf ("%s\n%s",
					  _("PCMCIA initialisation failed"),
					  PCMCIA_ERROR);
		gpe_error_box (errmsg);
		free (errmsg);
		free (PCMCIA_ERROR);
	}
	else
	{
		get_driver_list ();
		gtk_timeout_add (500, (GtkFunction) do_update, NULL);
	}
	return bookbox;
}
