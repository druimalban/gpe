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

#define _GNU_SOURCE

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
#include <sys/time.h>
#include <time.h>

#include "xsi.h"

#undef CHECK_RC

typedef struct _Key Key;

struct _Key
{
  int keycode;
  unsigned int modifier;
  char *command;
  int type; /* 0 - on release; 1 - on late release; 2 - on press */
  Key *next;
  char *window;
};

struct key_event
{
  XEvent ev;
  struct timeval time;
  struct key_event *next;
};

Display *dpy;
Window root;
Key *key = NULL;
Key *lastkey = NULL;
int NumLockMask, CapsLockMask, ScrollLockMask;
#ifdef CHECK_RC
time_t last_update;
#endif
struct timeval key_press_time;
char *rc_file;

struct key_event *keys_down;

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

#ifdef CHECK_RC
time_t
get_last_update (char *rc_file)
{
  struct stat stat_buf;

  /* If no rc file then set time equal to last_update */
  if (stat (rc_file, &stat_buf))
    stat_buf.st_mtime = last_update;
  return stat_buf.st_mtime;
}
#endif

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
      if (modifiers != AnyModifier)
        {
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
}

void
create_new_key (char *key_string)
{
  Key *k, *prevkey;
  char *key_str;
  char *sep1, *sep2;

#ifdef DEBUG
  printf ("create_new_key: %s\n", key_string);
#endif

  sep1 = strchr (key_string, ':');
  if (sep1 == NULL)
    {
      fprintf (stderr, "Invalid line in file: %s\n", key_string);
      exit (1);
    }

  if ((k = malloc (sizeof (*k))) == NULL)
    {
      fprintf (stderr, "Unable to allocate memory for key!\n");
      exit (1);
    }
  k->next = NULL;

  if (key == NULL)
    {
      key = k;
      prevkey = NULL;
    }
  else
    {
      lastkey->next = k;
      prevkey = lastkey;
    }

  lastkey = k;

  k->modifier = 0;
  k->keycode = 0;

  key_str = strndup (key_string, sep1 - key_string);

  sep2 = strchr (&sep1[1], ':');

  if (sep2 == NULL)
    {
      k->window = NULL;
      k->command = strdup (&sep1[1]);
    }
  else
    {
      k->window = strndup (&sep1[1], sep2 - sep1 - 1);
      k->command = strdup (&sep2[1]);
    }

  if (strlen (key_str) < 4)
    {
      fprintf (stderr, "No key specified: '%s'\n", key_string);
      exit (1);
    }

  if (key_str[0] == '?')
    {
      k->modifier = AnyModifier;
    }
  else
    {
      if (key_str[0] == '*')
	k->modifier = k->modifier | ShiftMask;
      if (key_str[1] == '*')
	k->modifier = k->modifier | ControlMask;
      if (key_str[2] == '*')
	k->modifier = k->modifier | Mod1Mask;
    }

  if ((strlen (key_str) > 11) && (strncmp (key_str+3, "Pressed ", 8) == 0))
    {
      k->keycode = XKeysymToKeycode (dpy, XStringToKeysym (key_str + 11));
      k->type = 2;
    }
  else if ((strlen (key_str) > 8) && (strncmp (key_str+3, "Held ", 5) == 0))
    {
      k->keycode = XKeysymToKeycode (dpy, XStringToKeysym (key_str + 8));
      k->type = 1;
    }
  else
    {
      k->keycode = XKeysymToKeycode (dpy, XStringToKeysym (key_str + 3));
      k->type = 0;
    }

  if (k->keycode == 0)
    {
      fprintf (stderr, "Keysym not found, for key definition '%s'\n", key_str);
      free (key_str);
      free (k->window);
      free (k->command);
      free (k);
      if (prevkey)
	{
	  prevkey->next = NULL;
	  lastkey = prevkey;
	}
      else
	{
	  key = NULL;
	  lastkey = NULL;
	}
      return;
    }

  grab_key (k->keycode, k->modifier, root);

  free (key_str);

#ifdef DEBUG
  fprintf (stderr, "new key: '%s' '%s' (%X) (%d) (%d)\n", k->window, k->command, k->keycode, k->modifier, k->type);
#endif
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
  char buf[1024];

#ifdef DEBUG
  printf ("parse_rc: %s\n", rc_file);
#endif

  if ((rc = fopen (rc_file, "r")))
    {
      while (fgets (buf, sizeof buf, rc))
	{
	  if (!strncmp (buf, "key=", 4))
	    {
	      if (buf[strlen(buf)-1] == '\n')
		{
		  buf[strlen(buf)-1] = 0;
		}
	      create_new_key (&buf[4]);
	    }
	}
      fclose (rc);
    }
#ifdef CHECK_RC
  last_update = get_last_update (rc_file);
#endif
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
  fprintf (stderr, "signal_handler\n");
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
initialize (int argc, char *argv[])
{
  struct sigaction act;
  int i;
  int rcflag = 0;

  act.sa_handler = signal_handler;
  act.sa_flags = 0;
  sigaction (SIGTERM, &act, NULL);
  sigaction (SIGINT, &act, NULL);
  sigaction (SIGHUP, &act, NULL);
  sigaction (SIGCHLD, &act, NULL);

  for (i = 1; i < argc; i++)
    {
      if (rcflag)
	{
	  rc_file = argv[i];
	  rcflag = 0;
	}
      else if (!strcmp (argv[i], "-f"))
	rcflag = 1;
    }

  if (!(dpy = XOpenDisplay (NULL)))
    print_error ("Can't open display, X may not be running.\n", True);

  root = XDefaultRootWindow (dpy);

  init_keyboard ();
  
  if (! rc_file)
    rc_file = find_rc ();

  parse_rc (rc_file);
}

#ifdef DEBUG
static void print_key (XEvent ev)
{
  KeySym t;
  char *t2;
  t = XKeycodeToKeysym (dpy, ev.xkey.keycode, 0);
  t2 = XKeysymToString (t);
  printf ("key: %s (%X %lx) (%d)\n", t2, ev.xkey.keycode, t,
	  ev.xkey.state);
}
#endif

/*
 *
 * Main
 *
 */

void process_key (XEvent ev, int type)
{
  Key *k;

	  ev.xkey.state =
	    ev.xkey.state & (Mod1Mask | ControlMask | ShiftMask);
	  for (k = key; k != NULL; k = k->next)
	    if (k->keycode == ev.xkey.keycode
		&& (k->modifier == AnyModifier
		    || k->modifier == ev.xkey.state)
		&& (k->type == type))
	      {
	        if (k->window == NULL
		    || (try_to_raise_window (dpy, k->window) != 0))
		  {
#ifdef DEBUG
		    fprintf (stderr, "running program\n");
#endif
		    fork_exec (k->command);
		  }

		k = lastkey;
	      }
}

int
main (int argc, char *argv[])
{
  initialize (argc, argv);

  signal (SIGCHLD, SIG_IGN);

  while (1)
    {
      XEvent ev;

      while (!XPending (dpy) && keys_down)
	{
	  int fd = ConnectionNumber (dpy);
	  fd_set fds;
          struct timeval time_now, time_required;
	  struct key_event *k = keys_down;

	  gettimeofday (&time_now, NULL);
	  timersub (&k->time, &time_now, &time_required);

	  FD_ZERO (&fds);
	  FD_SET (fd, &fds);

	  if (select (fd + 1, &fds, NULL, NULL, &time_required) > 0)
	    break;

#ifdef DEBUG
	  fprintf (stderr, "timeout\n");
	  print_key (k->ev);
#endif

	  process_key (k->ev, 1);
	  
	  if (keys_down)
	  keys_down = k->next;
	  free (k);
	}

      XNextEvent (dpy, &ev);
      if (ev.type == KeyPress)
        {
	  struct key_event *k;
          struct timeval time_now;
          struct timeval time_elapsed;
#ifdef DEBUG
          fprintf (stderr, "key press event...\n");
	  print_key (ev);
#endif

	  gettimeofday (&time_now, NULL);

	  timersub (&time_now, &key_press_time, &time_elapsed);
	  if ((time_elapsed.tv_sec == 0) && (time_elapsed.tv_usec < 500000))
	    {
#ifdef DEBUG
	      fprintf (stderr, "ignoring - too soon after previous\n");
#endif
	      key_press_time.tv_sec = 0;
	      key_press_time.tv_usec = 0;
	    }
          else
	    {
	      key_press_time = time_now;
	    }

	  k = malloc (sizeof (struct key_event));
	  memcpy (&k->ev, &ev, sizeof (ev));
	  k->next = NULL;
	  time_elapsed.tv_sec = 0;
	  time_elapsed.tv_usec = 500000;
	  timeradd (&time_now, &time_elapsed, &k->time);

	  if (keys_down)
	    {
	      struct key_event *p;
	      for (p = keys_down; p->next; p = p->next)
		p->next = k;
	    }
	  else
	    keys_down = k;

	  process_key (ev, 2);

	}
      else if (ev.type == KeyRelease
	       && (key_press_time.tv_sec || key_press_time.tv_usec))
	{
	  int type = 1;
	  struct key_event **kp;

#ifdef DEBUG
	  fprintf (stderr, "key release event...\n");
	  print_key (ev);
#endif

	  for (kp = &keys_down; *kp; kp = &(*kp)->next)
	    {
	      struct key_event *k = *kp;
	      if (k->ev.xkey.keycode == ev.xkey.keycode)
		{
		  type = 0;
		  *kp = k->next;
		  free (k);
		  break;
		}
	    }

#ifdef DEBUG
	  if (type == 0)
	    fprintf (stderr, "short press\n");
          else
	    fprintf (stderr, "long press\n");
#endif

	  if (type == 0)
	    process_key (ev, type);

          free_keys ();
          XTestFakeKeyEvent (dpy, ev.xkey.keycode, True, 0);
	  XTestFakeKeyEvent (dpy, ev.xkey.keycode, False, 0);
#ifdef DEBUG
	  printf ("faking key event\n");
#endif
	  parse_rc (rc_file);
	}

#ifdef CHECK_RC
      if (get_last_update (rc_file) != last_update)
	{
	  free_keys ();
	  parse_rc (rc_file);
	}
#endif

    }
  return 0;
}
