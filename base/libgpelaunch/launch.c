/*
 * gpe-appmgr - a program launcher
 *
 * Copyright (c) 2004 Phil Blundell
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
 */

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <libsn/sn-launcher.h>

#include <gpe/errorbox.h>

#include "config.h"
#include "launch.h"

#define MAX_ARGS 255

struct sn_display_map
{
  Display *dpy;
  SnDisplay *sn_dpy;
};

GSList *sn_display_list;

static GdkFilterReturn
sn_event_filter (GdkXEvent *xevp, GdkEvent *ev, gpointer p)
{
  XEvent *xev = (XEvent *)xevp;
  
  if (sn_display_process_event ((SnDisplay *)p, xev))
    return GDK_FILTER_REMOVE;

  return GDK_FILTER_CONTINUE;
}

static SnDisplay *
sn_display_for_display (Display *dpy)
{
  GSList *l;
  struct sn_display_map *map;

  for (l = sn_display_list; l; l = l->next)
    {
      map = l->data;
      if (map->dpy == dpy)
	return map->sn_dpy;
    }

  map = g_malloc (sizeof (*map));
  map->dpy = dpy;
  map->sn_dpy = sn_display_new (dpy, NULL, NULL);
  sn_display_list = g_slist_prepend (sn_display_list, map);
  gdk_window_add_filter (GDK_ROOT_PARENT (), sn_event_filter, map->sn_dpy);
  return map->sn_dpy;
}

/* Exec a command like the shell would.  */
static int 
do_exec (char *cmd)
{
  char *p = cmd;
  char *buf = alloca (strlen (cmd) + 1), *bufp = buf;
  char *argv[MAX_ARGS + 1];
  int nargs = 0;
  int escape = 0, squote = 0, dquote = 0;
  int rc;
  int i;

  if (cmd[0] == 0)
    {
      errno = ENOENT;
      return -1;
    }
  
  while (*p)
    {
      if (escape)
	{
	  *bufp++ = *p;
	  escape = 0;
	}
      else
	{
	  switch (*p)
	    {
	    case '\\':
	      escape = 1;
	      break;
	    case '"':
	      if (squote)
		*bufp++ = *p;
	      else
		dquote = !dquote;
	      break;
	    case '\'':
	      if (dquote)
		*bufp++ = *p;
	      else
		squote = !squote;
	      break;
	    case ' ':
	      if (!squote && !dquote)
		{
		  *bufp = 0;
		  if (nargs < MAX_ARGS)
		    argv[nargs++] = strdup (buf);
		  bufp = buf;
		  break;
		}
	    default:
	      *bufp++ = *p;
	      break;
	    }
	}
      p++;
    }
  
  if (bufp != buf)
    {
      *bufp = 0;
      if (nargs < MAX_ARGS)
	argv[nargs++] = strdup (buf);
    }

  argv[nargs] = NULL;
  rc = execvp (argv[0], argv);

  for (i = 0; i < nargs; i++)
    free (argv[i]);

  return rc;
}

gboolean
gpe_launch_program (Display *dpy, char *exec, char *name)
{
  SnLauncherContext *context;
  SnDisplay *sn_dpy;
  int screen;
  pid_t pid;

  screen = DefaultScreen (dpy);
  sn_dpy = sn_display_for_display (dpy);
  context = sn_launcher_context_new (sn_dpy, screen);
  
  if (name)
    sn_launcher_context_set_name (context, name);
  sn_launcher_context_set_binary_name (context, exec);

  sn_launcher_context_initiate (context, "gpe-appmgr launch", exec, CurrentTime);

  pid = fork ();
  switch (pid)
    {
    case -1:
      gpe_perror_box ("fork");
      return FALSE;

    case 0:
      sn_launcher_context_setup_child_process (context);
      do_exec (exec);
      gpe_perror_box ("exec");
      _exit (1);
      
    default:
      break;
    }

  return TRUE;
}
