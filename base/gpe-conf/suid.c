/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
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

#include "suid.h"
#include "applets.h"
#include "gpe/pixmaps.h"
#include "gpe/render.h"
#include "gpe/picturebutton.h"
#include "login-setup.h"

static GtkWidget *passwd_entry;
static int retv;

int check_root_passwd (const char *passwd);

int check_user_access (const char *cmd);

#ifdef __DARWIN__		// to compile/test under powerpc macosx
int
stime (time_t * t)
{
  printf ("Setting time : %ld uid:%d euid:%d\n", *t, getuid (), geteuid ());
  return (geteuid () != 0) ? -1 : 0;
}
#endif

void
update_login_bg_sh (char * setupinfo)
{
	if (atoi(setupinfo))
      system_printf("rm -f %s", GPE_LOGIN_BG_DONTSHOW_FILE);
    else 
	  system_printf("touch %s", GPE_LOGIN_BG_DONTSHOW_FILE);
}

void
update_ownerinfo_sh (char * setupinfo)
{
	if (atoi(setupinfo)) 
		system_printf("rm -f %s", GPE_OWNERINFO_DONTSHOW_FILE);
  	else 
		system_printf("touch %s", GPE_OWNERINFO_DONTSHOW_FILE);
}

void 
update_login_lock_auto (char * setupinfo)
{
  if (atoi(setupinfo)) 
	system_printf("chmod a+x %s", GPE_LOGIN_LOCK_SCRIPT);
  else
    system_printf("chmod a-x %s", GPE_LOGIN_LOCK_SCRIPT);
}

void 
update_login_background (char * file)
{
  system_printf("rm -f %s",GPE_LOGIN_BG_LINKED_FILE);
  if (symlink(file,GPE_LOGIN_BG_LINKED_FILE))
	  g_warning("Could not create link to %s\n",file);
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

  GError *err;

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
	This will check if user is allowed to execute desired command. 
*/
int
check_user_access (const char *cmd)
{
  if (!geteuid ())
    return TRUE;
  return FALSE;			// this !allows everything
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
      fflush (stdout);
      fscanf (in, "%4s\n%s\n", cmd, passwd);
      printf ("cmd: \"%s\"\n", cmd);
      //    if (!feof (in))
      {
	cmd[4] = 0;
	bin = NULL;
	if ((check_user_access (cmd) == TRUE) || (check_root_passwd (passwd)))	// we want to know it exact
	  {
	    /* if (strcmp (cmd, "CHEK") == 0)  we do nothing - just check root password */

	    if (strcmp (cmd, "NTPD") == 0)
	      {
		bin = "/usr/sbin/ntpdate";
		sprintf (arg1, "-b");
		fscanf (in, "%100s", arg2);
		numarg = 2;
	      }
	    else if (strcmp (cmd, "STIM") == 0)
	      {
		time_t t;
		fscanf (in, "%ld", &t);
		if (stime (&t) == -1)
		  fprintf (stderr, "error while setting the time: %d\n",
			   errno);
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
