/* 
 * Oroborus Window Manager - KeyLaunch (patched for GPE)
 *
 * Copyright (C) 2001 Ken Lynch
 * Copyright (C) 2002 Stefan Pfetzing
 * Copyright (C) 2002 Moray Allan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "xsi.h"

typedef struct _Key Key;

struct _Key
{
  int keycode, modifier;
  char *command;
  Key *next;
  char *window;
};

Display *dpy;
Window root;
Key *key = NULL;
Key *lastkey = NULL;
int NumLockMask, CapsLockMask, ScrollLockMask;
time_t last_update;
char *rc_file;

int previousevent_keycode = 0;
Time previousevent_time = 0;

/*
 *
 * General functions
 *
 */

void
print_error (char *error, int critical)
{
#ifdef DEBUG
  printf ("print_error\n");
#endif

  fprintf (stderr, "KEYLAUNCH: %s", error);
  if (critical)
    exit (1);
}

time_t
get_last_update (char *rc_file)
{
  struct stat stat_buf;

  /* If no rc file then set time equal to last_update */
  if (stat (rc_file, &stat_buf))
    stat_buf.st_mtime = last_update;
  return stat_buf.st_mtime;
}

/*
 *
 * Key functions
 *
 */

void
init_keyboard ()
{
  XModifierKeymap *xmk = NULL;
  KeyCode *map;
  int m, k;

#ifdef DEBUG
  printf ("init_keyboard\n");
#endif

  xmk = XGetModifierMapping (dpy);
  if (xmk)
    {
      map = xmk->modifiermap;
      for (m = 0; m < 8; m++)
	for (k = 0; k < xmk->max_keypermod; k++, map++)
	  {
	    if (*map == XKeysymToKeycode (dpy, XK_Num_Lock))
	      NumLockMask = (1 << m);
	    if (*map == XKeysymToKeycode (dpy, XK_Caps_Lock))
	      CapsLockMask = (1 << m);
	    if (*map == XKeysymToKeycode (dpy, XK_Scroll_Lock))
	      ScrollLockMask = (1 << m);
	  }
      XFreeModifiermap (xmk);
    }
}

void
grab_key (int keycode, unsigned int modifiers, Window w)
{
  if (keycode)
    {
      XGrabKey (dpy, keycode, modifiers, w, True, GrabModeAsync,
		GrabModeAsync);
      XGrabKey (dpy, keycode, modifiers | NumLockMask, w, False,
		GrabModeAsync, GrabModeAsync);
      XGrabKey (dpy, keycode, modifiers | CapsLockMask, w, False,
		GrabModeAsync, GrabModeAsync);
      XGrabKey (dpy, keycode, modifiers | ScrollLockMask, w, False,
		GrabModeAsync, GrabModeAsync);
      XGrabKey (dpy, keycode, modifiers | NumLockMask | CapsLockMask, w,
		False, GrabModeAsync, GrabModeAsync);
      XGrabKey (dpy, keycode, modifiers | NumLockMask | ScrollLockMask, w,
		False, GrabModeAsync, GrabModeAsync);
      XGrabKey (dpy, keycode,
		modifiers | NumLockMask | CapsLockMask | ScrollLockMask, w,
		False, GrabModeAsync, GrabModeAsync);
    }
}

void
create_new_key (char *key_string)
{
  Key *k;
  char *key_str, *command = NULL, *temp = NULL, *window = NULL;

#ifdef DEBUG
  printf ("create_new_key\n");
#endif

  key_str = strtok (key_string, ":");
  if (key_str != NULL)
    {
      temp = strtok (NULL, ":");
      if (temp != NULL)
	{
	  window = temp;
	  command = strtok (NULL, "\n");
	}
    }

  if (key_str == NULL || command == NULL)
    return;

  if (strlen (key_str) < 4)
    return;

  if ((k = malloc (sizeof *k)) == NULL)
    return;
  k->next = key;
  key = k;

  if (lastkey == NULL)
    lastkey = k;

  k->modifier = 0;
  k->keycode = 0;

  if (key_str[0] == '*')
    k->modifier = k->modifier | ShiftMask;
  if (key_str[1] == '*')
    k->modifier = k->modifier | ControlMask;
  if (key_str[2] == '*')
    k->modifier = k->modifier | Mod1Mask;

  k->keycode = XKeysymToKeycode (dpy, XStringToKeysym (key_str + 3));

  grab_key (k->keycode, k->modifier, root);
  k->command = strdup (command);
  if (window)
    k->window = strdup (window);
  else
    k->window = NULL;
  return;
}

void
free_keys ()
{
  Key *k;

#ifdef DEBUG
  printf ("free_keys\n");
#endif

  XUngrabKey (dpy, AnyKey, AnyModifier, root);
  while (key)
    {
      if (key->command)
	free (key->command);
      if (key->window)
	free (key->window);
      k = key->next;
      free (key);
      key = k;
    }
  lastkey = NULL;
}

/*
 *
 * Rc file functions
 *
 */

char *
find_rc ()
{
  FILE *rc;
  char *rc_file1;

  if ((rc_file1 =
       malloc (strlen (getenv ("HOME")) + strlen (RCFILE1) + 2)) == NULL)
    {
      perror ("malloc");
      exit (1);
    }

  sprintf (rc_file1, "%s/%s", getenv ("HOME"), RCFILE1);

  if ((rc = fopen (rc_file1, "r")))
    {
      fclose (rc);
      return rc_file1;
    }

  return RCFILE2;
}

void
parse_rc (char *rc_file)
{
  FILE *rc;
  char buf[1024], *lvalue, *rvalue;

#ifdef DEBUG
  printf ("parse_rc\n");
#endif

  if ((rc = fopen (rc_file, "r")))
    {
      while (fgets (buf, sizeof buf, rc))
	{
	  lvalue = strtok (buf, "=");
	  if (lvalue)
	    {
	      if (!strcmp (lvalue, "key"))
		{
		  rvalue = strtok (NULL, "\n");
		  if (rvalue)
		    create_new_key (rvalue);
		}
	    }
	}
      fclose (rc);
    }
  last_update = get_last_update (rc_file);
}

/*
 *
 * Execute command
 *
 */

void
fork_exec (char *cmd)
{
  pid_t pid = fork ();

#ifdef DEBUG
  printf ("fork_exec\n");
  printf ("  executing %s\n", cmd);
#endif

  switch (pid)
    {
    case 0:
      execlp ("/bin/sh", "sh", "-c", cmd, NULL);
      fprintf (stderr, "Exec failed.\n");
      exit (0);
      break;
    case -1:
      fprintf (stderr, "Fork failed.\n");
      break;
    }
}

/*
 *
 * Initialize and quit functions
 *
 */

void
quit ()
{
#ifdef DEBUG
  printf ("quit\n");
#endif

  free_keys ();
  XCloseDisplay (dpy);
  exit (0);
}

void
signal_handler (int signal)
{
#ifdef DEBUG
  printf ("signal_handler\n");
#endif

  switch (signal)
    {
    case SIGINT:
    case SIGTERM:
    case SIGHUP:
      quit ();
      break;
    case SIGCHLD:
      wait (NULL);
      break;
    }
}

void
initialize ()
{
  struct sigaction act;

  act.sa_handler = signal_handler;
  act.sa_flags = 0;
  sigaction (SIGTERM, &act, NULL);
  sigaction (SIGINT, &act, NULL);
  sigaction (SIGHUP, &act, NULL);
  sigaction (SIGCHLD, &act, NULL);

  if (!(dpy = XOpenDisplay (NULL)))
    print_error ("Can't open display, X may not be running.\n", True);

  root = XDefaultRootWindow (dpy);

  init_keyboard ();

  rc_file = find_rc ();

  parse_rc (rc_file);
}

/*
 *
 * Main
 *
 */

int
main (int argc, char *argv[])
{
  initialize ();

  while (1)
    {
      while (XPending (dpy))
	{
	  XEvent ev;
	  Key *k;

	  waitpid (-1, NULL, WNOHANG);

	  XNextEvent (dpy, &ev);
	  if (ev.type == KeyPress)
	    {
	      ev.xkey.state =
		ev.xkey.state & (Mod1Mask | ControlMask | ShiftMask);
	      for (k = key; k != NULL; k = k->next)
		if (k->keycode == ev.xkey.keycode
		    && k->modifier == ev.xkey.state
		    && (k->window == NULL
			|| (ev.xkey.keycode != previousevent_keycode
			    || abs (ev.xkey.
				    time - previousevent_time) > 1000)))
		  {
		    if (k->window == NULL
			|| (try_to_raise_window (dpy, k->window) != 0))
		      {
			printf ("running program\n");
			fork_exec (k->command);
		      }
		  }
	      previousevent_keycode = ev.xkey.keycode;
	      previousevent_time = ev.xkey.time;
	    }
	  else if (ev.type == KeyRelease)
	    {
	      ev.xkey.state =
		ev.xkey.state & (Mod1Mask | ControlMask | ShiftMask);
	      for (k = key; k != NULL; k = k->next)
		if (k->keycode == ev.xkey.keycode
		    && k->modifier == ev.xkey.state)
		  {
		    printf ("key release event...\n");
		    free_keys ();
		    XTestFakeKeyEvent (dpy, ev.xkey.keycode, True, 0);
		    XTestFakeKeyEvent (dpy, ev.xkey.keycode, False, 0);
		    printf ("faking key event\n");
		    parse_rc (rc_file);
		    k = lastkey;
		  }
	    }
	  /*
	     if (get_last_update (rc_file) != last_update)
	     {
	     free_keys ();
	     parse_rc (rc_file);
	     }
	   */
	}
      usleep (1);
    }
  return 0;
}
