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

#include "suid.h"
#include "applets.h"
#include "gpe/pixmaps.h"
#include "gpe/render.h"
#include "gpe/picturebutton.h"


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

int
execlp1 (char *a1, char *a2, char *a3, char *a4)
{
  printf ("%s %s %s\n", a1, a2, a3);
  return 0;
}

int
execlp2 (char *a1, char *a2, char *a3, char *a4, char *a5)
{
  printf ("%s %s %s %s\n", a1, a2, a3, a4);
  return 0;
}


/*
	This will check if user is allowed to execute desired command. 
*/
int check_user_access(const char *cmd)
{
	if (!geteuid()) return TRUE;
	return FALSE; // this !allows everything
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
      fscanf (in, "%4s\n%s\n", cmd,passwd);
  printf("cmd: \"%s\"\n",cmd);
  //    if (!feof (in))
	{
	  cmd[4] = 0;
	  bin = NULL;
      if ((check_user_access(cmd) == TRUE) || (check_root_passwd(passwd))) // we want to know it exact
	  {
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
			  fprintf (stderr, "error while setting the time: %d\n", errno);
	
			}
	
		  /* of course it is a security hole */
		  /* but certainly enough for PDA..  */
	
		  else if (strcmp (cmd, "CPPW") == 0)
			{
			  bin = "/bin/cp";
			  strcpy (arg1, "/tmp/passwd");
			  strcpy (arg2, "/etc/passwd");
			  numarg = 2;
			  system_printf("/bin/cp %s %s",arg1 ,arg2);
			  system_printf("chmod 0644 %s",arg2);
			  system_printf("/bin/rm -f %s",arg1);
			}
		  else if (strcmp (cmd, "XCAL") == 0)
			{
			  system ("/usr/X11R6/bin/xcalibrate");
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
				  execlp (bin, strrchr(bin,'/')+1, arg1, NULL);
				  break;
				case 2:
				  execlp (bin, strrchr(bin,'/')+1, arg1, arg2, NULL);
				  break;
				}
			  exit (0);
			default:
			  break;
			}
			}
#endif			
		} // if check_user_access
		else // clear buffer
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
	} //if !feof
    }
//	gtk_exit(0);
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
