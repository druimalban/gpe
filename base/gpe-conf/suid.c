/*

 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *               2003  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
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
#include "ipaqscreen/brightness.h"
#include "ipaqscreen/rotation.h"

static GtkWidget *passwd_entry;
static int retv;
static int global_user_access = FALSE;
static int know_global_user_access = FALSE;

int check_root_passwd (const char *passwd);
int check_user_access (const char *cmd);

void update_light_status (int state)
{
	if ((state >= 0) && (state <=1))
	turn_light(state);
}

void update_screen_brightness (int br)
{
	set_brightness (br);
}

void update_screen_rotation (int rotation)
{
	if ((rotation >= 0) && (rotation <= 3)) 
		set_rotation(rotation);
}


void update_dhcp_status(const gchar* active)
{
	gint use_dhcp = atoi(active);
	
	if (use_dhcp)
	{
		change_cfg_value("/etc/pcmcia/network.opts","DHCP","\"y\"",'=');
		switch (fork ())
		  {
		  case -1:
		    fprintf (stderr, "cant fork\n");
		    exit (errno);
		  case 0:
			execlp ("/sbin/dhcpcd", "dhcpcd", "-d", "eth0");
		    exit(0);
		  default:
		  break;
	      }
	}
	else
	{
		change_cfg_value("/etc/pcmcia/network.opts","DHCP","\"n\"",'=');
		system("/usr/bin/killall dhcpcd");
	}
}

void update_dns_server(const gchar* server)
{
	change_cfg_value("/etc/resolv.conf","nameserver",server,' ');
}

void update_time_from_net(const gchar* server)
{
	if (system_printf("/usr/sbin/ntpdate -b %s",server))
	{
		fprintf(stderr,"failed to execute ntpdate\n");
	}
	else // if ok, update rtc time
	{
		system("echo > /var/spool/at/trigger");
	}
}


void
change_cfg_value (const gchar * file, const gchar * var, const gchar * val, gchar seperator)
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
	  fprintf(stderr,"Could not access file: %s.\n",file);
	  if (access(file,F_OK)) // file exists, but access is denied
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
	  j=get_first_char(delim);
	  if (j>0) {
		  tmpval = g_malloc(j);
		  strncpy(tmpval,delim,j);
	  	  lines[i] = g_strdup_printf ("%s%s%c%s", tmpval,var,seperator,val);
	  	  g_free(tmpval);
	  }
	  else
	  {
	  	lines[i] = g_strdup_printf ("%s%c%s", var,seperator, val);
	  }
	}
      i++;
    }

  i--;

writefile:
	
  if ((delim == NULL) && val)
    {
      lines = realloc (lines, (i+1) * sizeof (gchar *));
      lines[i] = g_strdup_printf ("%s%c%s", var,seperator,val);
      i++;
      lines[i] = NULL;
    }
  else
    free (delim);
  
  fnew = fopen (file, "w");
  if (!fnew) 
  {
     fprintf(stderr,"Could not write to file: %s.\n",file);
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

void
set_timezone (gchar * zone)
{
  gchar *profile;
  gchar **proflines;
  gint length;
  gchar *delim;
  FILE *profnew;
  gint i = 0;
  gint j = 0;

  GError *err = NULL;

  delim = g_strdup ("\n");
  g_file_get_contents ("/etc/profile", &profile, &length, &err);
  proflines = g_strsplit (profile, delim, 255);
  g_free (delim);
  delim = NULL;
  g_free (profile);

  while (proflines[i])
    {
      if ((g_strrstr (proflines[i], "TZ"))
	  && (g_strrstr (proflines[i], "export"))
	  && (!g_strrstr (proflines[i], "#")))
	{
	  delim = proflines[i];
	  proflines[i] = g_strdup_printf ("export TZ=%s", zone);
	}
      i++;
    }

  i--;

  if (delim == NULL)
    {
      proflines = realloc (proflines, i * sizeof (gchar *));
      proflines[i] = g_strdup_printf ("export TZ=%s", zone);
      i++;
    }
  else
    free (delim);

  profnew = fopen ("/etc/profile", "w");

  for (j = 0; j < i; j++)
    {
      fprintf (profnew, "%s\n", proflines[j]);
    }
  fclose (profnew);
  g_strfreev (proflines);
}

/* 
	Creating a new homedir for a user.
*/
void
create_homedir(char *dir, char *user)
{
	struct passwd *pwent;

	printf("dir: %s\n",dir);
	printf("user: %s\n",user);
	if (g_str_has_prefix(dir,"/home/"))
	{
		pwent = getpwnam(user);
		if (pwent)
		{
			mkdir(dir,S_IRWXU);
			chown(dir,pwent->pw_uid,pwent->pw_gid);
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
  if (!know_global_user_access) // read this only once
  {
  	gchar* acstr;
	acstr = get_file_var(GPE_CONF_CFGFILE,"user_access");
	if (acstr==NULL)
		acstr = "0";
	global_user_access = atoi(acstr);
	know_global_user_access = TRUE;
  }	
  // allow screen settings
  if (!strcmp(cmd,"SCRR")) return TRUE;
  if (!strcmp(cmd,"SCRB")) return TRUE;
	  
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
  char arg1[100];
  char arg2[100];
  int numarg = 0;

  setuid (0);

  while (!feof (in))		// the prg exits with sigpipe
    {
      cmd[0] = ' ';
      fflush (stdout);
      fscanf (in, "%4s\n%s\n", cmd, passwd);
      //    if (!feof (in))
      {
	cmd[4] = 0;
	bin = NULL;
	if ((check_user_access (cmd) == TRUE) || (check_root_passwd (passwd)))	// we want to know it exact
	  {
	    /* if (strcmp (cmd, "CHEK") == 0)  we do nothing - just check root password */

	    if (strcmp (cmd, "NTPD") == 0)
	      {
		    fscanf (in, "%100s", arg2); // get timeserver
			update_time_from_net(arg2);
	      }
	    else if (strcmp (cmd, "STIM") == 0)
	      {
		time_t t;
		fscanf (in, "%ld", &t);
		if (stime (&t) == -1)
		  fprintf (stderr, "error while setting the time: %d\n",
			   errno);
		else // if ok, update rtc time
		{
			system("echo > /var/spool/at/trigger");
		}
	      }
	    /* of course it is a security hole */
	    /* but certainly enough for PDA..  */
	    else if (strcmp (cmd, "CPPW") == 0)
	      {
		bin = "/bin/cp";
		strcpy (arg1, "/tmp/passwd");
		strcpy (arg2, "/etc/passwd");
		numarg = 2;
		system_printf ("/bin/cp %s %s", arg1, arg2);
		system_printf ("chmod 0644 %s", arg2);
		system_printf ("/bin/rm -f %s", arg1);
	      }
	    else if (strcmp (cmd, "CPIF") == 0)
	      {
		bin = "/bin/cp";
		strcpy (arg1, "/tmp/interfaces");
		strcpy (arg2, "/etc/network/interfaces");
		numarg = 2;
		system_printf ("/bin/cp %s %s", arg1, arg2);
		system_printf ("chmod 0644 %s", arg2);
		system_printf ("/bin/rm -f %s", arg1);
		system ("/etc/init.d/networking restart");
	      }
	    else if (strcmp (cmd, "CPOI") == 0)
	      {
		bin = "/bin/cp";
		system_printf ("/bin/cp %s %s", GPE_OWNERINFO_TMP, GPE_OWNERINFO_DATA);
		system_printf ("chmod 0644 %s", GPE_OWNERINFO_DATA);
		system_printf ("/bin/rm -f %s", GPE_OWNERINFO_TMP);
	      }
	    else if (strcmp (cmd, "XCAL") == 0)
	      {
		system ("/usr/X11R6/bin/xcalibrate");
	      }
	    else if (strcmp (cmd, "STZO") == 0)
	      {
		fscanf (in, "%100s", arg2);
		set_timezone (arg2);
	      }
	    else if (strcmp (cmd, "ULBS") == 0)
	      {
		fscanf (in, "%1s", arg2);
		update_login_bg_sh (arg2);
	      }
	    else if (strcmp (cmd, "UOIS") == 0)
	      {
		fscanf (in, "%1s", arg2);
		update_ownerinfo_sh (arg2);
	      }
	    else if (strcmp (cmd, "ULDS") == 0)
	      {
		fscanf (in, "%1s", arg2);
		update_login_lock_auto (arg2);
	      }
	    else if (strcmp (cmd, "ULBF") == 0)
	      {
		fscanf (in, "%100s", arg2);
		update_login_background (arg2);
	      }
	    else if (strcmp (cmd, "DHCP") == 0)
	      {
		fscanf (in, "%100s", arg2);
		update_dhcp_status (arg2);
	      }
	    else if (strcmp (cmd, "SDNS") == 0)
	      {
		fscanf (in, "%100s", arg2);
		update_dns_server (arg2);
	      }
	    else if (strcmp (cmd, "SCRB") == 0)
	      {
		fscanf (in, "%d", &numarg);
		update_screen_brightness (numarg);
	      }
	    else if (strcmp (cmd, "SCRR") == 0)
	      {
		fscanf (in, "%d", &numarg);
		update_screen_rotation (numarg);
	      }
	    else if (strcmp (cmd, "SCRL") == 0)
	      {
		fscanf (in, "%d", &numarg);
		update_light_status (numarg);
	      }
	    else if (strcmp (cmd, "CRHD") == 0)
	      {
		fscanf (in, "%100s", arg1); // directory
		fscanf (in, "%100s", arg2); // username
		create_homedir (arg1, arg2);
	      }
#if 0
	    if (bin)		// fork and exec
	      {
		int PID;
		switch (PID = fork ())
		  {
		  case -1:
		    fprintf (stderr, "cant fork\n");
		    exit (errno);
		  case 0:
		    switch (numarg)
		      {
		      case 1:
			execlp (bin, strrchr (bin, '/') + 1, arg1, NULL);
			break;
		      case 2:
			execlp (bin, strrchr (bin, '/') + 1, arg1, arg2,
				NULL);
			break;
		      }
		    exit (0);
		  default:
		    break;
		  }
	      }
#endif
	  }			// if check_user_access
	else			// clear buffer
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
      }				//if !feof
    }
//      gtk_exit(0);
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
	  if (strcmp (crypt (passwd, pwent->pw_passwd), pwent->pw_passwd) ==
	      0)
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

#ifdef __DARWIN__		// to compile/test under powerpc macosx
int
stime (time_t * t)
{
  printf ("Setting time : %ld uid:%d euid:%d\n", *t, getuid (), geteuid ());
  return (geteuid () != 0) ? -1 : 0;
}
#endif
