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

#include <X11/Xlib.h>
#include <X11/Xatom.h>

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
  GSList *client_map;
  Atom client_map_atom;
  GSList *pending_startups;
};

struct launch_record
{
  launch_callback_func cb;
  void *data;
  SnLauncherContext *context;
};

struct startup_record
{
  const gchar *startup_id;
  const gchar *binary;
  time_t time;
};

static GSList *sn_display_list;

struct client_map
{
  Window window;
  const gchar *binary;
};

static void read_client_map (struct sn_display_map *map);
static void kill_pending_startup (struct sn_display_map *map, const char *id);

gboolean
gpe_launch_startup_is_pending (Display *dpy, const gchar *binary)
{
  GSList *l;
  struct sn_display_map *map;

  for (l = sn_display_list; l; l = l->next)
    {
      map = l->data;
      if (map->dpy == dpy)
	break;
    }

  if (l == NULL)
    return None;

  for (l = map->pending_startups; l; l = l->next)
    {
      struct startup_record *r = l->data;

      if (! strcmp (r->binary, binary))
	{
	  time_t t;
	  time (&t);
	  if ((t - r->time) > 10)
	    {
	      kill_pending_startup (map, r->startup_id);
	      return FALSE;
	    }
	  return TRUE;
	}
    }

  return FALSE;
}

Window
gpe_launch_get_window_for_binary (Display *dpy, const gchar *binary)
{
  GSList *l;
  struct sn_display_map *map;

  for (l = sn_display_list; l; l = l->next)
    {
      map = l->data;
      if (map->dpy == dpy)
	break;
    }

  if (l == NULL)
    return None;

  for (l = map->client_map; l; l = l->next)
    {
      struct client_map *c = l->data;
      if (! strcmp (c->binary, binary))
	return c->window;
    }
  
  return None;
}

gchar *
gpe_launch_get_binary_for_window (Display *dpy, Window w)
{
  GSList *l;
  struct sn_display_map *map;

  for (l = sn_display_list; l; l = l->next)
    {
      map = l->data;
      if (map->dpy == dpy)
	break;
    }

  if (l == NULL)
    return NULL;

  read_client_map (map);

  for (l = map->client_map; l; l = l->next)
    {
      struct client_map *c = l->data;
      if (c->window == w)
	return g_strdup (c->binary);
    }
  
  return NULL;
}

static void
read_client_map (struct sn_display_map *map)
{
  Atom type;
  int format;
  long bytes_after;
  unsigned char *data = NULL;
  long n_items;
  int result;
  unsigned char *p, *key = NULL, *value = NULL;
  GSList *l;

  /* flush old data */
  for (l = map->client_map; l; l = l->next)
    {
      struct client_map *c = l->data;
      g_free ((gchar *)c->binary);
      g_free (c);
    }

  g_slist_free (map->client_map);
  map->client_map = NULL;

  result =  XGetWindowProperty (map->dpy, DefaultRootWindow (map->dpy),
				map->client_map_atom,
				0, 10000L,
				False, XA_STRING,
				&type, &format, &n_items,
				&bytes_after, (unsigned char **)&data);

  if (result != Success || data == NULL || n_items == 0)
    {
      if (data) 
	XFree (data);
      return;
    }

  p = data;

  while (*p != '\0')
    {
      struct client_map *c;

      key = p;
      while (*p != '=' && *p != '\0') p++;

      if (*p == 0)
	return;

      *p = '\0'; p++;

      if (*p == 0)
	return;

      value = p;

      while (*p != '|' && *p != '\0') p++;

      if (*p == 0)
	return;

      *p = '\0';

      c = g_malloc (sizeof (*c));
      c->binary = g_strdup (key);
      c->window = atoi (value);
      map->client_map = g_slist_prepend (map->client_map, c);

      p++;
    }

  XFree (data);  
}

static void
client_map_process_event (struct sn_display_map *map, XEvent *xev)
{
  if (xev->type == PropertyNotify
      && xev->xproperty.window == DefaultRootWindow (xev->xany.display)
      && xev->xproperty.atom == map->client_map_atom)
    read_client_map (map);
}

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

  if (sn_dpy)
    {
      if (sn_display_process_event (sn_dpy, xev))
	return GDK_FILTER_REMOVE;

      client_map_process_event (map, xev);
    }

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
  map->client_map_atom = XInternAtom (dpy, "_MB_CLIENT_EXEC_MAP", False);
  map->client_map = NULL;
  map->pending_startups = NULL;
  read_client_map (map);
  sn_display_list = g_slist_prepend (sn_display_list, map);
  return map->sn_dpy;
}

static void
kill_pending_startup (struct sn_display_map *map, const char *id)
{
  GSList *l;
  struct startup_record *sr;

  for (l = map->pending_startups; l; l = l->next)
    {
      sr = l->data;

      if (!strcmp (sr->startup_id, id))
	{
	  map->pending_startups = g_slist_remove (map->pending_startups, sr);
	  g_free ((void *)sr->startup_id);
	  g_free ((void *)sr->binary);
	  g_free (sr);
	  return;
	}
    }
}

static void
global_monitor_event (SnMonitorEvent *event, void *user_data)
{
  SnMonitorContext *context;
  SnStartupSequence *sequence;
  const char *id;
  struct sn_display_map *map = user_data;
  struct startup_record *sr;

  context = sn_monitor_event_get_context (event);
  sequence = sn_monitor_event_get_startup_sequence (event);
  id = sn_startup_sequence_get_id (sequence);

  switch (sn_monitor_event_get_type (event))
    {
    case SN_MONITOR_EVENT_INITIATED:
      sr = g_malloc (sizeof (*sr));
      sr->binary = g_strdup (sn_startup_sequence_get_binary_name (sequence));
      sr->startup_id = g_strdup (id);
      time (&sr->time);
      map->pending_startups = g_slist_prepend (map->pending_startups, sr);
      break;
      
    case SN_MONITOR_EVENT_COMPLETED:
      kill_pending_startup (map, id);
      break;
      
    case SN_MONITOR_EVENT_CANCELED:
      kill_pending_startup (map, id);
      break;
    }
}

void
gpe_launch_monitor_display (Display *dpy)
{
  GSList *l;
  struct sn_display_map *map;
  SnDisplay *sn_dpy;

  sn_dpy = sn_display_for_display (dpy);
  
  for (l = sn_display_list; l; l = l->next)
    {
      map = l->data;
      if (map->dpy == dpy)
	sn_monitor_context_new (sn_dpy, DefaultScreen (dpy), global_monitor_event, map, NULL);
    }
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
      perror (exec);
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

