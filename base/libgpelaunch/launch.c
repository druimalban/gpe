/*
 * GPE program launcher library
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
#include <signal.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <libsn/sn-launcher.h>
#include <libsn/sn-monitor.h>

#include <gpe/errorbox.h>

#include "config.h"
#include "gpe/launch.h"

#define MAX_ARGS 255

struct sn_display_map
{
  Display *dpy;
  SnDisplay *sn_dpy;
};

struct launch_record
{
  launch_callback_func cb;
  void *data;
  SnLauncherContext *context;
};

static GSList *sn_display_list;

static GdkFilterReturn
sn_event_filter (GdkXEvent *xevp, GdkEvent *ev, gpointer p)
{
  XEvent *xev = (XEvent *)xevp;
  SnDisplay *sn_dpy = NULL;
  GSList *l;
  struct sn_display_map *map;

  for (l = sn_display_list; l; l = l->next)
    {
      map = l->data;
      if (map->dpy == xev->xany.display)
	{
	  sn_dpy = map->sn_dpy;
	  break;
	}
    }

  if (sn_dpy && sn_display_process_event (sn_dpy, xev))
    return GDK_FILTER_REMOVE;

  return GDK_FILTER_CONTINUE;
}

void
gpe_launch_install_filter (void)
{
  gdk_window_add_filter (NULL, sn_event_filter, NULL);
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
  return map->sn_dpy;
}

static void
monitor_event (SnMonitorEvent *event, void *user_data)
{
  SnMonitorContext *context;
  SnStartupSequence *sequence;
  const char *id;
  struct launch_record *r;

  context = sn_monitor_event_get_context (event);
  sequence = sn_monitor_event_get_startup_sequence (event);
  id = sn_startup_sequence_get_id (sequence);

  r = (struct launch_record *)user_data;
  if (!strcmp (sn_launcher_context_get_startup_id (r->context), id))
    {
      switch (sn_monitor_event_get_type (event))
	{
	case SN_MONITOR_EVENT_INITIATED:
	  r->cb (LAUNCH_STARTING, r->data);
	  break;

	case SN_MONITOR_EVENT_COMPLETED:
	  r->cb (LAUNCH_COMPLETE, r->data);
	  sn_monitor_context_unref (context);
	  break;
	  
	case SN_MONITOR_EVENT_CANCELED:
	  r->cb (LAUNCH_FAILED, r->data);
	  sn_monitor_context_unref (context);
	  break;
	} 
    }
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

static void
launch_record_free (void *ptr)
{
  struct launch_record *r = (struct launch_record *)ptr;
  sn_launcher_context_unref (r->context);
  g_free (r);
}

gboolean
gpe_launch_program_with_callback (Display *dpy, char *exec, char *name, gboolean startup_notify, launch_callback_func cb, void *data)
{
  SnLauncherContext *context = NULL;
  SnDisplay *sn_dpy = NULL;
  int screen;
  pid_t pid;
  int i;

  if (startup_notify)
    {
      screen = DefaultScreen (dpy);
      sn_dpy = sn_display_for_display (dpy);
      context = sn_launcher_context_new (sn_dpy, screen);
  
      if (name)
	sn_launcher_context_set_name (context, name);
      sn_launcher_context_set_binary_name (context, exec);

      if (cb)
	{
	  struct SnMonitorContext *monitor;
	  struct launch_record *r = g_malloc (sizeof (*r));
	  sn_launcher_context_ref (context);
	  r->cb = cb;
	  r->data = data;
	  r->context = context;
	  monitor = sn_monitor_context_new (sn_dpy, screen, monitor_event, r, launch_record_free);
	}

      sn_launcher_context_initiate (context, "libgpelaunch", exec, CurrentTime);
    }

  pid = fork ();
  switch (pid)
    {
    case -1:
      gpe_perror_box ("fork");
      return FALSE;

    case 0:
      /* Clear out signal dispositions.  GnomeVFS will segfault if invoked
	 with SIGCHLD set to SIG_IGN.  */
      for (i = 0; i < _NSIG; i++)
	signal (i, SIG_DFL);
      if (startup_notify)
	sn_launcher_context_setup_child_process (context);
      do_exec (exec);
      gpe_perror_box ("exec");
      _exit (1);
      
    default:
      break;
    }

  return TRUE;
}

gboolean
gpe_launch_program (Display *dpy, char *exec, char *name)
{
  return gpe_launch_program_with_callback (dpy, exec, name, FALSE, NULL, NULL);
}

