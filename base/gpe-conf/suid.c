/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *               2003,2004  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Loop for suid root jobs.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <crypt.h>
#include <glib.h>
#include <sys/stat.h>

#include "suid.h"
#include "applets.h"
#include "login-setup.h"
#include "gpe-admin.h"
#include "cfgfile.h"
#include "ownerinfo.h"
#include "screen/brightness.h"
#ifndef DISABLE_XRANDR
#include "screen/rotation.h"
#endif
#ifdef PACKAGETOOL		
#include "packages.h"
#include "keyboard.h"
#endif
#include "serial.h"
#include "cardinfo.h"
#include "timeanddate.h"
#include "users/passwd.h"

static GtkWidget *passwd_entry;
static int retv;
static int global_user_access = FALSE;
static int know_global_user_access = FALSE;

int check_root_passwd (const char *passwd);
int check_user_access (const char *cmd);


#ifdef PACKAGETOOL		
void
update_packages ()
{
	do_package_update ();
}
#endif

void
update_screen_brightness (int br)
{
	set_brightness (br);
}

#ifndef DISABLE_XRANDR
void
update_screen_rotation (int rotation)
{
	if ((rotation >= 0) && (rotation <= 3))
		set_rotation (rotation);
}
#endif

void
update_dns_server (const gchar * server)
{
	change_cfg_value ("/etc/resolv.conf", "nameserver", server, ' ');
}

void
update_time_from_net (const gchar * server)
{
	char *tstr = g_strdup_printf ("ntpdate -b %s", server);
	if (system(tstr))
	{
		fprintf (stderr, "failed to execute ntpdate\n");
	}
	else			// if ok, update rtc time
	{
		system ("echo > /var/spool/at/trigger");
	}
	g_free(tstr);
}


/* Changes a setting in a config file, to be used with caution.
 * seperator == 0 means to comment out this line 
 */

void
change_cfg_value (const gchar * file, const gchar * var, const gchar * val,
		  gchar seperator)
{
	gchar *content, *tmpval;
	gchar **lines = NULL;
	gint length;
	gchar *delim;
	FILE *fnew;
	gint i = 0;
	gint j = 0;
	GError *err = NULL;

	tmpval = "";
	delim = g_strdup ("\n");
	if (!g_file_get_contents (file, &content, &length, &err))
	{
		fprintf (stderr, "Could not access file: %s.\n", file);
		if (access (file, F_OK))	// file exists, but access is denied
		{
			i = 0;
			delim = NULL;
			goto writefile;
		}
	}
	lines = g_strsplit (content, delim, 2048);
	g_free (delim);
	delim = NULL;
	g_free (content);

	while (lines[i])
	{
		if ((g_strrstr (g_strchomp (lines[i]), var))
		    && (!g_str_has_prefix (g_strchomp (lines[i]), "#")))
		{
			delim = lines[i];
			if (seperator)
			{
				j = get_first_char (delim);
				if (j > 0)
				{
					tmpval = g_malloc (j+2);
					strncpy (tmpval, delim, j);
					tmpval[j]=0;
					lines[i] =
						g_strdup_printf ("%s%s%c%s", tmpval,
								 var, seperator, val);
					g_free (tmpval);
				}
				else
				{
					lines[i] =
						g_strdup_printf ("%s%c%s", var,
								 seperator, val);
				}
			}
			else
			{
				lines[i] = g_strdup_printf ("# %s", lines[i]);
			}
		}
		i++;
	}

	if (i)
		i--;

      writefile:

	if ((delim == NULL) && val)
	{
		lines = realloc (lines, (i + 1) * sizeof (gchar *));
		lines[i] = g_strdup_printf ("%s%c%s", var, seperator, val);
		i++;
		lines[i] = NULL;
	}
	else
		free (delim);

	fnew = fopen (file, "w");
	if (!fnew)
	{
		fprintf (stderr, "Could not write to file: %s.\n", file);
		return;
	}

	for (j = 0; j < i; j++)
	{
		fprintf (fnew, "%s\n", lines[j]);
	}
	fclose (fnew);
	g_strfreev (lines);
}


void
update_login_bg_sh (char *setupinfo)
{
	if (atoi (setupinfo))
		system_printf ("rm -f %s", GPE_LOGIN_BG_DONTSHOW_FILE);
	else
		system_printf ("touch %s", GPE_LOGIN_BG_DONTSHOW_FILE);
}

void
update_ownerinfo_sh (char *setupinfo)
{
	if (atoi (setupinfo))
		system_printf ("rm -f %s", GPE_OWNERINFO_DONTSHOW_FILE);
	else
		system_printf ("touch %s", GPE_OWNERINFO_DONTSHOW_FILE);
}

void
update_login_lock_auto (char *setupinfo)
{
	if (atoi (setupinfo))
		system_printf ("chmod a+x %s", GPE_LOGIN_LOCK_SCRIPT);
	else
		system_printf ("chmod a-x %s", GPE_LOGIN_LOCK_SCRIPT);
}

void
update_login_background (char *file)
{
	system_printf ("rm -f %s", GPE_LOGIN_BG_LINKED_FILE);
	if (symlink (file, GPE_LOGIN_BG_LINKED_FILE))
		g_warning ("Could not create link to %s\n", file);
}


/* 
	Creating a new homedir for a user.
*/
void
create_homedir (char *dir, char *user)
{
	struct passwd *pwent;

	if (g_str_has_prefix (dir, "/home/"))
	{
		pwent = getpwnam (user);
		if (pwent)
		{
			mkdir (dir, S_IRWXU);
			chown (dir, pwent->pw_uid, pwent->pw_gid);
		}
	}
}


/*
	This will check if user is allowed to execute desired command. 
*/
int
check_user_access (const char *cmd)
{
	if (!geteuid ())
		return TRUE;
	if (!know_global_user_access)	// read this only once
	{
		gchar *acstr;
		acstr = get_file_var (GPE_CONF_CFGFILE, "user_access");
		if (acstr == NULL)
			acstr = "0";
		global_user_access = atoi (acstr);
		know_global_user_access = TRUE;
	}
	
	/* allow screen settings */
	if (!strcmp (cmd, "SCRR"))
		return TRUE;
	if (!strcmp (cmd, "SCRB"))
		return TRUE;
	if (!strcmp (cmd, "SCRL"))
		return TRUE;

	/* allow time settings */
	if (!strcmp (cmd, "STIM"))
		return TRUE;
	if (!strcmp (cmd, "NTPD"))
		return TRUE;
	
	/* allow changing password */
	if (!strcmp (cmd, "CHPW"))
		return TRUE;
	
	return global_user_access;
}


/* this is a very simple way to do the stuff */
/* this avoid the gpe-confsuid bin I dislike.. */
void
suidloop (int write, int read)
{
	char cmd[5];
	char passwd[100];
	FILE *in = fdopen (read, "r");
	char *bin = NULL;
	char arg1[255];
	char arg2[100];
	int numarg = 0;
	int numarg2 = 0;

	setuid (0);
	nsreturnfd = write;
	nsreturn = fdopen (write, "w");

	g_type_init ();

	while (!feof (in))	// the prg exits with sigpipe
	{
		cmd[0] = ' ';
		fflush (stdout);
		fscanf (in, "%4s\n%s\n", cmd, passwd);
		{
			cmd[4] = 0;
			bin = NULL;
			if ((check_user_access (cmd) == TRUE) || (check_root_passwd (passwd)))	// we want to know it exact
			{

				if (strcmp (cmd, "NTPD") == 0)  // sets system time from timeserver
				{
					fscanf (in, "%100s", arg2);	// get timeserver
					update_time_from_net (arg2);
				}
				else if (strcmp (cmd, "STIM") == 0)  // sets the system time
				{
					time_t t;
					fscanf (in, "%ld", &t);
					if (stime (&t) == -1)
						fprintf (stderr,
							 "Error while setting the time: %d\n",
							 errno);
					else	// if ok, update rtc time
					{
						system ("echo > /var/spool/at/trigger");
					}
				}
				/* of course it is a security hole */
				/* but certainly enough for PDA..  */
				else if (strcmp (cmd, "CPPW") == 0)  // installs a new passwd file
				{
					bin = "/bin/cp";
					strcpy (arg1, "/tmp/passwd");
					strcpy (arg2, "/etc/passwd");
					numarg = 2;
					system_printf ("/bin/cp %s %s", arg1, arg2);
					system_printf ("chmod 0644 %s", arg2);
					system_printf ("/bin/rm -f %s", arg1);
				}
				else if (strcmp (cmd, "CPIF") == 0)  // installs a new interfaces file
				{
					fscanf (in, "%100s", arg2);	// to forget soon...
					system ("/usr/bin/killall dhcpcd");
					strcpy (arg1, "/tmp/interfaces");
					strcpy (arg2, "/etc/network/interfaces");
					system_printf ("/bin/cp %s %s", arg1,arg2);
					system_printf ("chmod 0644 %s", arg2);
					system_printf ("/bin/rm -f %s", arg1);
					system ("/etc/init.d/networking restart");
				}
				else if (strcmp (cmd, "CPOI") == 0)  // rewrites owner information data
				{
					bin = "/bin/cp";
					system_printf ("/bin/cp %s %s",
						       GPE_OWNERINFO_TMP,
						       GPE_OWNERINFO_DATA);
					system_printf ("chmod 0644 %s",
						       GPE_OWNERINFO_DATA);
					system_printf ("/bin/rm -f %s",
						       GPE_OWNERINFO_TMP);
				}
				else if (strcmp (cmd, "XCAL") == 0)  // runs screen calibration tool
				{
					system ("/usr/bin/xtscal");
				}
				else if (strcmp (cmd, "STZO") == 0)  // changes the timezone setting 
				{
					fscanf (in, "%100s", arg2);
					set_timezone ("/etc/profile",arg2);
				}
				else if (strcmp (cmd, "ULBS") == 0)  // turn login bg on/off
				{
					fscanf (in, "%1s", arg2);
					update_login_bg_sh (arg2);
				}
				else if (strcmp (cmd, "UOIS") == 0)  // change if owner information is shown or not
				{
					fscanf (in, "%1s", arg2);
					update_ownerinfo_sh (arg2);
				}
				else if (strcmp (cmd, "ULDS") == 0)  // change auto lock of display
				{
					fscanf (in, "%1s", arg2);
					update_login_lock_auto (arg2);
				}
				else if (strcmp (cmd, "ULBF") == 0)  // change background of login screen
				{
					fscanf (in, "%100s", arg2);
					update_login_background (arg2);
				}
				else if (strcmp (cmd, "SDNS") == 0)  // change to another dns server
				{
					fscanf (in, "%100s", arg2);
					update_dns_server (arg2);
				}
				else if (strcmp (cmd, "SCRB") == 0)  // change frontlight brightness
				{
					fscanf (in, "%d", &numarg);
					update_screen_brightness (numarg);
				}
#ifndef DISABLE_XRANDR
				else if (strcmp (cmd, "SCRR") == 0)  // rotate screen
				{
					fscanf (in, "%d", &numarg);
					update_screen_rotation (numarg);
				}
#endif				
				else if (strcmp (cmd, "CRHD") == 0)  // create a users home directory
				{
					fscanf (in, "%100s", arg1);	// directory
					fscanf (in, "%100s", arg2);	// username
					create_homedir (arg1, arg2);
				}
#ifdef PACKAGETOOL		
				else if (strcmp (cmd, "NWUD") == 0)  // run system packages update
				{
					fscanf (in, "%100s", arg1);
					update_packages ();
				}
				else if (strcmp (cmd, "NWIS") == 0)  // run package installation
				{
					fscanf (in, "%100s", arg1);
					do_package_install (arg1,FALSE);
				}
				else if (strcmp (cmd, "NWRM") == 0)  // run package uninstall
				{
					fscanf (in, "%100s", arg1);
					do_package_install (arg1,TRUE);
				}
				else if (strcmp (cmd, "PAIS") == 0)  // install a ipk package
				{
					fscanf (in, "%100s", arg1);
					do_package_install (arg1,FALSE);
				}
#endif				
				else if (strcmp (cmd, "SERU") == 0)  // change serial port usage
				{
					fscanf (in, "%d", &numarg);
					assign_serial_port (numarg);
				}
				else if (strcmp (cmd, "SGPS") == 0)  // change gps settings
				{
					fscanf (in, "%100s %100s", arg1, arg2);
					update_gpsd_settings(arg1+1, (arg1[0] == '1') ? 1 : 0, arg2);
				}
				else if (strcmp (cmd, "CMID") == 0)  // identify pcmcia card
				{
					fscanf (in, "%d", &numarg);
					do_get_card_ident (numarg);
				}
				else if (strcmp (cmd, "CMCO") == 0)  // do pcmcia ioctl
				{
					fscanf (in, "%d %d", &numarg, &numarg2);
					do_ioctl (numarg,numarg2);
				}
				else if (strcmp (cmd, "CMRE") == 0)  // reset cardmgr
				{
					fscanf (in, "%100s", arg1);
					do_reset ();
				}
				else if (strcmp (cmd, "CMCP") == 0)  // copy pcmcia config
				{
					fscanf (in, "%d", &numarg); // numarg==1 tells us the destination is wlan-ng.conf
					if (numarg)
					{
						system_printf ("/bin/cp %s %s",
						       pcmcia_tmpcfgfile,
						       wlan_ng_cfgfile);
						system_printf ("chmod 0644 %s",
						       wlan_ng_cfgfile);
						system_printf ("/bin/rm -f %s",
						       pcmcia_tmpcfgfile);
					}
					else
					{
						system_printf ("/bin/cp %s %s",
						       pcmcia_tmpcfgfile,
						       pcmcia_cfgfile);
						system_printf ("chmod 0644 %s",
						       pcmcia_cfgfile);
						system_printf ("/bin/rm -f %s",
						       pcmcia_tmpcfgfile);
					}
				}
#ifdef PACKAGETOOL		
				else if (strcmp (cmd, "KBDC") == 0)  // write a new keyboard config file
				{
					fscanf (in, "%255s", arg1);
					write_keyboard_cfg (arg1);
				}
#endif				
				else if (strcmp (cmd, "GAUA") == 0)  // change user access to gpe-conf
				{
					fscanf (in, "%100s", arg1);
					do_change_user_access (arg1);
				}
				else if (strcmp (cmd, "DHDI") == 0)  // delete home directory
				{
					gchar *cmd;
					fscanf (in, "%100s", arg1);
					cmd = g_strdup_printf("rm -rf /home%s", arg1);
					system (cmd);
					g_free(cmd);
				}
				else if (strcmp (cmd, "CHPW") == 0)  // change users password
				{
					struct passwd *pw;
					
					fscanf (in, "%100s %128s", arg2, arg1);
					/* empty password */
					if (!strcmp(arg1, "*"))
						memset(arg1, 0, sizeof(arg1));
					pw = getpwnam(arg2);
					if (pw)
						update_passwd (pw, arg1);
				}
			}	// if check_user_access
			else	// clear buffer
			{
				if (strcmp (cmd, "NTPD") == 0)
				{
					fscanf (in, "%100s", arg2);
				}
				else if (strcmp (cmd, "STIM") == 0) 
				{
					time_t t;
					fscanf (in, "%ld", &t);
				}
			}
		}		//if !feof
	}
	fclose (in);
	fclose (nsreturn);
	exit (0);
}

int
check_root_passwd (const char *passwd)
{
	struct passwd *pwent;
	setpwent ();
	pwent = getpwent ();
	while (pwent)
	{
		if (strcmp (pwent->pw_name, "root") == 0)
		{
			if (strcmp
			    (crypt (passwd, pwent->pw_passwd),
			     pwent->pw_passwd) == 0)
				return 1;
			else
				return 0;
		}
		pwent = getpwent ();
	}
	return 0;
}

void
verify_pass (gpointer user_data)
{
	if (check_root_passwd (gtk_entry_get_text (GTK_ENTRY (passwd_entry))))
	{
		retv = 1;
		gtk_widget_destroy (GTK_WIDGET (user_data));	// close the dialog
	}
}

gboolean
no_root_passwd (void)
{
	struct passwd *pwent;
	setpwent ();
	pwent = getpwent ();
	while (pwent)
	{
		if (strcmp (pwent->pw_name, "root") == 0)
		{
			if (!pwent->pw_passwd || !strlen(pwent->pw_passwd))
				return TRUE;
			else
				return FALSE;
		}
		pwent = getpwent ();
	}
	return FALSE;
}

#ifdef __DARWIN__		// to compile/test under powerpc macosx
int
stime (time_t * t)
{
	printf ("Setting time : %ld uid:%d euid:%d\n", *t, getuid (),
		geteuid ());
	return (geteuid () != 0) ? -1 : 0;
}
#endif
