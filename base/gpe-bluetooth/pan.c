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
#include <unistd.h>
#include <stdio.h>
#include <libintl.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <sys/socket.h>

#include <gtk/gtk.h>
#include <gpe/errorbox.h>
#include <gpe/tray.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include "main.h"
#include "pan.h"
#include "bnep.h"

#define _(x) (x)

extern int bnep_init (void);

struct bt_service_pan
{
  struct bt_service service;

  struct bt_device *bd;

  GThread *thread;
  pid_t pid;
  volatile gboolean terminate;
  GtkWidget *w;
  int fd;
};

/* Wait for disconnect or error condition on the socket */
static int 
wait_for_hup (struct bt_service_pan *svc)
{
  struct pollfd pf;
  int n;

  while (!svc->terminate) 
    {
      pf.fd = svc->fd;
      pf.events = POLLERR | POLLHUP;
      n = poll (&pf, 1, -1);
      if (n < 0) 
	{
	  if (errno == EINTR || errno == EAGAIN)
	    continue;
	  return 1;
	}
      
      if (n) 
	{
	  int err = 0, olen = sizeof (err);
	  getsockopt (svc->fd, SOL_SOCKET, SO_ERROR, &err, &olen);

	  close (svc->fd);
	  return -err;
	}
    }

  return 0;
}

/* Connect and initiate BNEP session
 * Returns:
 *   -1 - critical error (exit persist mode)
 *   1  - non critical error
 *   0  - success
 */
static int 
create_connection (bdaddr_t *bdaddr)
{
  struct l2cap_options l2o;
  struct sockaddr_l2 l2a;
  int sk, olen;
  
  sk = socket (AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
  if (sk < 0) 
    {
      perror ("socket");
      return -1;
    }
  
  /* Setup L2CAP options according to BNEP spec */
  if (getsockopt (sk, SOL_L2CAP, L2CAP_OPTIONS, &l2o, &olen))
    {
      perror ("getsockopt");
      goto error;
    }

  l2o.imtu = l2o.omtu = BNEP_MTU;

  if (setsockopt (sk, SOL_L2CAP, L2CAP_OPTIONS, &l2o, sizeof(l2o)))
    {
      perror ("setsockopt");
      goto error;
    }
  
  l2a.l2_family = AF_BLUETOOTH;
  
  /* Set local address */
  l2a.l2_psm = 0;
  l2a.l2_bdaddr = src_addr;

  if (bind (sk, (struct sockaddr *) &l2a, sizeof(l2a)))
    {
      perror ("bind");
      goto error;
    }
  
  l2a.l2_psm = htobs (BNEP_PSM);
  baswap (&l2a.l2_bdaddr, bdaddr);
  
  if (connect(sk, (struct sockaddr *) &l2a, sizeof(l2a)))
    {
      perror ("connect");
      goto error;
    }

  return sk;

error:
  close (sk);
  return -1;
}

struct __service_16 
{
  uint16_t dst;
  uint16_t src;
} __attribute__ ((packed));

/* Create BNEP connection 
 * sk      - Connect L2CAP socket
 * role    - Local role
 * service - Remote service
 * dev     - Network device (contains actual dev name on return)
 */
static int 
bnep_open_session (int sk, uint16_t role, uint16_t svc)
{
  struct bnep_setup_conn_req *req;
  struct bnep_control_rsp *rsp;
  struct __service_16 *s;
  unsigned char pkt[BNEP_MTU];
  int r;

  /* Send request */
  req = (void *) pkt;
  req->type = BNEP_CONTROL;
  req->ctrl = BNEP_SETUP_CONN_REQ;
  req->uuid_size = 2;	//16bit UUID
  s = (void *) req->service;
  s->dst = htons(svc);
  s->src = htons(role);
  
  if (send (sk, pkt, sizeof(*req) + sizeof(*s), 0) < 0)
    return -1;

receive:
  /* Get response */
  r = recv (sk, pkt, BNEP_MTU, 0);
  if (r <= 0)
    return -1;
  
  errno = EPROTO;
  
  if (r < sizeof (*rsp))
    return -1;
  
  rsp = (void *) pkt;
  if (rsp->type != BNEP_CONTROL)
    return -1;
  
  if (rsp->ctrl != BNEP_SETUP_CONN_RSP)
    goto receive;
  
  r = ntohs(rsp->resp);
  
  switch (r) {
  case BNEP_SUCCESS:
    break;
    
  case BNEP_CONN_INVALID_DST:
  case BNEP_CONN_INVALID_SRC:
  case BNEP_CONN_INVALID_SVC:
    errno = EPROTO;
    return -1;
    
  case BNEP_CONN_NOT_ALLOWED:
    errno = EACCES;
    return -1;
  }
  
  return 0;
}

static struct bt_service_desc pan_service_desc;

static struct bt_service *
pan_scan (sdp_record_t *rec, struct bt_device *bd)
{
  struct bt_service_pan *s;

  s = g_malloc (sizeof (*s));

  s->service.desc = &pan_service_desc;
  s->bd = bd;
  s->thread = NULL;
  s->fd = -1;
  s->terminate = FALSE;
  s->w = NULL;

  return (struct bt_service *)s;
}

static gboolean
pan_connect (struct bt_service_pan *svc, GError *error)
{
  char buf[16];
  pid_t pid;
  int status;

  svc->fd = create_connection (&svc->bd->bdaddr);
  if (svc->fd < 0)
    {
      return FALSE;
    }

  if (bnep_open_session (svc->fd, BNEP_SVC_PANU, BNEP_SVC_NAP))
    {
      close (svc->fd);
      svc->fd = -1;
      return FALSE;
    }

  sprintf (buf, "%d", svc->fd);

  pid = vfork ();
  if (pid == 0)
    {
      execl (PREFIX "/lib/gpe-bluetooth/bnep-helper", PREFIX "/lib/gpe-bluetooth/bnep-helper", buf, NULL);
      perror ("exec");
      _exit (1);
    }

  waitpid (pid, &status, 0);

  if (WEXITSTATUS (status))
    {
      close (svc->fd);
      svc->fd = -1;
      return FALSE;
    }
  
  return TRUE;
}

static void
pan_disconnect (bdaddr_t *bd)
{
  pid_t pid;
  int status;
  gchar *text;

  text = g_strdup (batostr (bd));
  
  pid = vfork ();
  if (pid == 0)
    {
      execl (PREFIX "/lib/gpe-bluetooth/bnep-helper", PREFIX "/lib/gpe-bluetooth/bnep-helper", "-k", text, NULL);
      perror ("exec");
      _exit (1);
    }

  g_free (text);

  waitpid (pid, &status, 0);
}

static void
usr1_handler (void)
{
}

static void
pan_thread (struct bt_service_pan *svc)
{
  char *text;
  GError error;
  gboolean rc;

  svc->pid = getpid ();
  signal (SIGUSR1, usr1_handler);

  text = g_strdup_printf (_("Connecting to %s"), batostr (&svc->bd->bdaddr));

  gdk_threads_enter ();
  svc->w = bt_progress_dialog (text, svc->bd->pixbuf);
  gtk_widget_show_all (svc->w);
  gdk_flush ();
  gdk_threads_leave ();
  g_free (text);

  rc = pan_connect (svc, &error);

  gdk_threads_enter ();
  gtk_widget_destroy (svc->w);
  gdk_flush ();
  gdk_threads_leave ();

  if (rc == FALSE)
    {
      gdk_threads_enter ();
      gpe_error_box_nonblocking (_("Connection failed\n"));
      gdk_threads_leave ();
      svc->thread = NULL;
      g_thread_exit (0);
    }

  for (;;)
    {
      int err;
      gchar *text;
      gboolean rc;
      guint id;
      
      err = wait_for_hup (svc);

      if (err == 0)
	{
	  fprintf (stderr, "pan: thread exiting.\n");
	  pan_disconnect (&svc->bd->bdaddr);
	  g_thread_exit (0);
	}

      if (err < 0)
	text = g_strdup_printf (_("PAN connection to %s lost: %s"),
				batostr (&svc->bd->bdaddr),
				strerror (-err));
      else
	text = g_strdup_printf (_("PAN connection to %s lost"),
				batostr (&svc->bd->bdaddr));

      gdk_threads_enter ();
      id = gpe_system_tray_send_message (dock_window, text, 0);
      gdk_threads_leave ();
      schedule_message_delete (id, 5000);

      g_free (text);

      do
	{
	  sleep (5);
	  rc = pan_connect (svc, &error);	  
	} while (rc == FALSE);

      text = g_strdup_printf (_("PAN connection to %s re-established"),
			      batostr (&svc->bd->bdaddr));
			      
      gdk_threads_enter ();
      id = gpe_system_tray_send_message (dock_window, text, 0);
      gdk_threads_leave ();
      schedule_message_delete (id, 5000);

      g_free (text);
    }
}

static void
pan_menu_connect (GtkWidget *w, struct bt_service_pan *svc)
{
  svc->thread = g_thread_create ((GThreadFunc) pan_thread, svc, FALSE, 0);
}

static void
pan_menu_disconnect (GtkWidget *w, struct bt_service_pan *svc)
{
  svc->terminate = TRUE;
  kill (svc->pid, SIGUSR1);
  svc->thread = NULL;
}

static void
pan_popup_menu (struct bt_service *svc, GtkWidget *menu)
{
  struct bt_service_pan *pan;
  GtkWidget *w;
  
  pan = (struct bt_service_pan *)svc;

  if (pan->thread)
    {
      w = gtk_menu_item_new_with_label (_("Disconnect PAN"));
      g_signal_connect (G_OBJECT (w), "activate", G_CALLBACK (pan_menu_disconnect), svc);
    }
  else
    {
      w = gtk_menu_item_new_with_label (_("Connect PAN"));
      g_signal_connect (G_OBJECT (w), "activate", G_CALLBACK (pan_menu_connect), svc);
    }

  gtk_widget_show (w);
  gtk_menu_append (GTK_MENU (menu), w);
}

void
pan_init (void)
{
  sdp_uuid16_create (&pan_service_desc.uuid, NAP_SVCLASS_ID);  

  pan_service_desc.scan = pan_scan;
  pan_service_desc.popup_menu = pan_popup_menu;

  service_desc_list = g_slist_prepend (service_desc_list, &pan_service_desc);
}
