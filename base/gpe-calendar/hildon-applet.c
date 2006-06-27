/* hildon-applet.c - GPE Calendar applet for Hildon.
   Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#include <hildon-home-plugin/hildon-home-plugin-interface.h>
#include <libosso.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <fcntl.h>

#include <linux/inotify.h>
#include <unistd.h>
#include "inotify-syscalls.h"

#include "globals.h"
#include "event-list.h"

#define WIDTH 400
#define HEIGHT 400

static osso_context_t *osso;

static EventDB *event_db;
static EventList *event_list;

static int inotify_fd;
static int calendar_watch;
static GIOChannel *inotify_channel;
static guint inotify_watch;

static gboolean
inotify_event (GIOChannel *source, GIOCondition condition, gpointer data)
{
  if ((condition & G_IO_IN) == 0)
    return TRUE;

  /* Read what we can.  We don't actually waste time looking at the
     contents: we have exactly one file we are watching and gratuitous
     reloads are not actually problematic.  */
  char buffer[4096];
  int len = read (inotify_fd, buffer, sizeof (buffer));
  if (len < 0)
    printf ("%s: read: %m\n", __func__);

  /* Queue an event reload.  */
  event_list_reload_events (event_list);

  /* Don't remove event.  */
  return TRUE;
}

static void
event_list_event_clicked (EventList *el, Event *ev, GdkEventButton *b,
			  gpointer data)
{
  if (b->button == 1)
    {
      char buffer[200];
      snprintf (buffer, sizeof (buffer),
		"VIEWTIME=%ld", event_get_start (ev));
      osso_application_top (osso, "gpe_calendar", buffer);
    }
}

void *
hildon_home_applet_lib_initialize (void *state_data,
				   int *state_size,
				   GtkWidget **widget)
{
  printf ("%s:%s (%d)\n", __FILE__, __func__, getpid ());

  osso = osso_initialize ("gpe_calendar_home", "0.1", FALSE, NULL);
  if (! osso)
    return NULL;

  char *filename = CALENDAR_FILE ();
  event_db = event_db_new (filename);
  if (! event_db)
    {
      g_critical ("Failed to open %s", filename);
      g_free (filename);
      return NULL;
    }

  inotify_fd = inotify_init ();
  if (inotify_fd < 0)
    g_warning ("Failed to initialize inotify."
	       " Unable to watch db for changes.");

  if (inotify_fd >= 0)
    {
      calendar_watch = inotify_add_watch (inotify_fd, filename, IN_MODIFY);
      if (calendar_watch < 0)
	g_warning ("Failed to create a watch for %s", filename);
    }
  else
    calendar_watch = -1;

  g_free (filename);

  if (calendar_watch >= 0)
    {
      inotify_channel = g_io_channel_unix_new (inotify_fd);
      inotify_watch = g_io_add_watch (inotify_channel, G_IO_IN,
				      inotify_event, NULL);
    }

  GtkWidget *frame = gtk_frame_new (NULL);
  *widget = frame;
  gtk_widget_show (frame);
  gtk_widget_set_size_request (frame, WIDTH, HEIGHT); 

  event_list = EVENT_LIST (event_list_new (event_db));
  event_list_set_period_box_visible (event_list, FALSE);
  g_signal_connect (event_list, "event-clicked",
		    G_CALLBACK (event_list_event_clicked), NULL);
  gtk_widget_show (GTK_WIDGET (event_list));
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (event_list));

  return NULL;
}

void
hildon_home_applet_lib_deinitialize (void *applet_data)
{
  if (osso)
    osso_deinitialize (osso);

  g_source_remove (inotify_watch);
  g_io_channel_unref (inotify_channel);
  inotify_rm_watch (inotify_fd, calendar_watch);
  close (inotify_fd);

  g_object_unref (event_db);
}

int
hildon_home_applet_lib_get_requested_width (void *data)
{
  return WIDTH;
}

int
hildon_home_applet_lib_save_state (void *applet_data,
				   void **state_data, int *state_size)
{
  /* We have nothing to save.  */
  *state_size = 0;
  return 1;
}

void
hildon_home_applet_lib_background (void *applet_data)
{
  return;
}

void
hildon_home_applet_lib_foreground (void *applet_data)
{
  return;
}

GtkWidget *
hildon_home_applet_lib_settings (void *data, GtkWindow *parent)
{
  return NULL;
}

