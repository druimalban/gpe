/*
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2003 Jamey Hicks <jamey@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>
#include <locale.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include <gtk/gtk.h>
#include <gpe/errorbox.h>
#include <gpe/tray.h>

#include "main.h"
#include "sdp.h"
#include "headset.h"
#include "rfcomm.h"

#define _(x) gettext(x)

struct bt_service_headset
{
  struct bt_service service;

  struct bt_device *bd;

  int port;
  
  GThread *thread;
  volatile gboolean terminate;
  GtkWidget *w;
  int fd;
  pid_t bluezsco_pid;
};

static struct bt_service_desc headset_service_desc;

static struct bt_service *
headset_scan (sdp_record_t *rec, struct bt_device *bd)
{
  struct bt_service_headset *s;
  int port;

  port = 6;

  s = g_malloc (sizeof (*s));

  s->service.desc = &headset_service_desc;
  s->bd = bd;
  s->port = port;
  s->thread = NULL;
  s->fd = -1;
  s->w = NULL;
  s->terminate = FALSE;

  return (struct bt_service *)s;
}

static gboolean
headset_connect (struct bt_service_headset *svc, GError *error)
{
  pid_t pid;
  char bdaddr_buf[32];
  char chan_buf[32];

  sprintf (chan_buf, "%d", svc->port);
  
  pid = vfork ();
  if (pid == 0)
    {
      ba2str(&svc->bd->bdaddr, bdaddr_buf);
      execlp (PREFIX "/bin/bluezsco", bdaddr_buf, chan_buf, NULL);
      perror ("exec");
      _exit (1);
    }
  
  svc->bluezsco_pid = pid;

  close (svc->fd);

  return TRUE;
}

static void
headset_thread (struct bt_service_headset *svc)
{
  char *text;
  GError error;
  gboolean rc;

  text = g_strdup_printf (_("Connecting to %s"), batostr (&svc->bd->bdaddr));

  gdk_threads_enter ();
  svc->w = bt_progress_dialog (text, svc->bd->pixbuf);
  gtk_widget_show_all (svc->w);
  gdk_flush ();
  gdk_threads_leave ();
  g_free (text);

  rc = headset_connect (svc, &error);

  gdk_threads_enter ();
  gtk_widget_destroy (svc->w);
  gdk_flush ();
  gdk_threads_leave ();

  if (rc == FALSE)
    {
      gdk_threads_enter ();
      gpe_error_box_nonblocking (_("Connection failed\n"));
      gdk_threads_leave ();
      return;
    }

  for (;;)
    {
      gchar *text;
      gboolean rc;
      guint id;
      int status;

      rc = waitpid (svc->bluezsco_pid, &status, 0);

      fprintf (stderr, "waitpid returns %d\n", rc);

      if (svc->terminate)
	g_thread_exit (0);

      if (rc > 0)
	{
	  text = g_strdup_printf (_("HEADSET connection to %s lost"),
			      batostr (&svc->bd->bdaddr));

	  gdk_threads_enter ();
	  id = gpe_system_tray_send_message (dock_window, text, 0);
	  gdk_threads_leave ();
	  schedule_message_delete (id, 5000);

	  g_free (text);
	  
	  do
	    {
	      sleep (5);
	      rc = headset_connect (svc, &error);	  
	    } while (rc == FALSE);
	  
	  text = g_strdup_printf (_("HEADSET connection to %s re-established"),
			      batostr (&svc->bd->bdaddr));
			      
	  gdk_threads_enter ();
	  id = gpe_system_tray_send_message (dock_window, text, 0);
	  gdk_threads_leave ();
	  schedule_message_delete (id, 5000);

	  g_free (text);
	}
    }
}

static void
headset_menu_connect (GtkWidget *w, struct bt_service_headset *svc)
{
  svc->thread = g_thread_create ((GThreadFunc) headset_thread, svc, FALSE, 0);
}

static void
headset_menu_disconnect (GtkWidget *w, struct bt_service_headset *svc)
{
  svc->terminate = TRUE;
  kill (svc->bluezsco_pid, SIGTERM);
  svc->thread = NULL;
}

static void
headset_popup_menu (struct bt_service *svc, GtkWidget *menu)
{
  struct bt_service_headset *headset;
  GtkWidget *w;

  headset = (struct bt_service_headset *)svc;

  if (headset->thread)
    {
      w = gtk_menu_item_new_with_label (_("Disconnect HEADSET"));
      g_signal_connect (G_OBJECT (w), "activate", G_CALLBACK (headset_menu_disconnect), svc);
    }
  else
    {
      w = gtk_menu_item_new_with_label (_("Connect HEADSET"));
      g_signal_connect (G_OBJECT (w), "activate", G_CALLBACK (headset_menu_connect), svc);
    }

  gtk_widget_show (w);
  gtk_menu_append (GTK_MENU (menu), w);
}

void
headset_init (void)
{
  sdp_uuid16_create (&headset_service_desc.uuid, HEADSET_SVCLASS_ID);

  headset_service_desc.scan = headset_scan;
  headset_service_desc.popup_menu = headset_popup_menu;

  service_desc_list = g_slist_prepend (service_desc_list, &headset_service_desc);
}
