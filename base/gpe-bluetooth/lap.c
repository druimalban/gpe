/*
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
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
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include <gtk/gtk.h>
#include <gpe/errorbox.h>

#include "main.h"
#include "sdp.h"
#include "lap.h"
#include "rfcomm.h"

#define _(x) gettext(x)

struct bt_service_lap
{
  struct bt_service service;

  struct bt_device *bd;

  int port;
  
  GThread *thread;
  gboolean terminate;
  GtkWidget *w;
  int fd;
  pid_t pppd_pid;
};

static struct bt_service_desc lap_service_desc;

static struct bt_service *
lap_scan (sdp_record_t *rec, struct bt_device *bd)
{
  struct bt_service_lap *s;
  int port;

  if (sdp_find_rfcomm (rec, &port) == FALSE)
    return NULL;

  s = g_malloc (sizeof (*s));

  s->service.desc = &lap_service_desc;
  s->bd = bd;
  s->port = port;
  s->thread = NULL;
  s->fd = -1;
  s->w = NULL;
  s->terminate = FALSE;

  return (struct bt_service *)s;
}

static gboolean
lap_connect (struct bt_service_lap *svc, GError *error)
{
  pid_t pid;
  char buf[32];

  if (rfcomm_connect (&svc->bd->bdaddr, svc->port, &svc->fd, error) == FALSE)
    return FALSE;

  sprintf (buf, "%d", svc->fd);
  
  pid = vfork ();
  if (pid == 0)
    {
      execlp (PREFIX "/lib/gpe-bluetooth/rfcomm-helper", "rfcomm-helper", buf, "ppp", NULL);
      perror ("exec");
      _exit (1);
    }
  
  svc->pppd_pid = pid;

  close (svc->fd);

  return TRUE;
}

static void
lap_thread (struct bt_service_lap *svc)
{
  char *text;
  GError error;
  gboolean rc;

  text = g_strdup_printf (_("Connecting to %s"), batostr (&svc->bd->bdaddr));

  gdk_threads_enter ();
  svc->w = bt_progress_dialog (text, svc->bd->pixbuf);
  gtk_widget_show_all (svc->w);
  gdk_threads_leave ();
  g_free (text);

  rc = lap_connect (svc, &error);

  gdk_threads_enter ();
  gtk_widget_destroy (svc->w);
  gdk_threads_leave ();

  if (rc == FALSE)
    {
      gdk_threads_enter ();
      gpe_error_box_nonblocking (_("Connection failed\n"));
      gdk_threads_leave ();
      return;
    }

#if 0
  for (;;)
    {
      int err;
      gchar *text;
      gboolean rc;
      guint id;
      
      err = wait_for_hup (svc);
      
      if (err < 0)
	text = g_strdup_printf (_("LAP connection to %s lost: %s"),
				batostr (&svc->bd->bdaddr),
				strerror (-err));
      else
	text = g_strdup_printf (_("LAP connection to %s lost"),
				batostr (&svc->bd->bdaddr));

      id = gpe_system_tray_send_message (dock_window, text, 0);
      schedule_message_delete (id, 5000);

      g_free (text);

      do
	{
	  sleep (5);
	  rc = lap_connect (svc, &error);	  
	} while (rc == FALSE);

      text = g_strdup_printf (_("LAP connection to %s re-established"),
			      batostr (&svc->bd->bdaddr));
			      
      id = gpe_system_tray_send_message (dock_window, text, 0);
      schedule_message_delete (id, 5000);

      g_free (text);
    }
#endif
}

static void
lap_menu_connect (GtkWidget *w, struct bt_service_lap *svc)
{
  svc->thread = g_thread_create ((GThreadFunc) lap_thread, svc, FALSE, 0);
}

static void
lap_menu_disconnect (GtkWidget *w, struct bt_service_lap *svc)
{
}

static void
lap_popup_menu (struct bt_service *svc, GtkWidget *menu)
{
  struct bt_service_lap *lap;
  GtkWidget *w;

  lap = (struct bt_service_lap *)svc;

  if (lap->thread)
    {
      w = gtk_menu_item_new_with_label (_("Disconnect LAP"));
      g_signal_connect (G_OBJECT (w), "activate", G_CALLBACK (lap_menu_disconnect), svc);
    }
  else
    {
      w = gtk_menu_item_new_with_label (_("Connect LAP"));
      g_signal_connect (G_OBJECT (w), "activate", G_CALLBACK (lap_menu_connect), svc);
    }

  gtk_widget_show (w);
  gtk_menu_append (GTK_MENU (menu), w);
}

void
lap_init (void)
{
  sdp_uuid16_create (&lap_service_desc.uuid, LAN_ACCESS_SVCLASS_ID);

  lap_service_desc.scan = lap_scan;
  lap_service_desc.popup_menu = lap_popup_menu;

  service_desc_list = g_slist_prepend (service_desc_list, &lap_service_desc);
}
