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

#include <gtk/gtk.h>

#include <sys/socket.h>
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
};

#if 0
/* Wait for disconnect or error condition on the socket */
static int w4_hup(int sk)
{
  struct pollfd pf;
  int n;

  while (!terminate) 
    {
      pf.fd = sk;
      pf.events = POLLERR | POLLHUP;
      n = poll(&pf, 1, -1);
      if (n < 0) 
	{
	  if (errno == EINTR || errno == EAGAIN)
	    continue;
	  syslog(LOG_ERR, "Poll failed. %s(%d)",
		 strerror(errno), errno);
	  return 1;
	}
      
      if (n) 
	{
	  int err = 0, olen = sizeof(err);
	  getsockopt(sk, SOL_SOCKET, SO_ERROR, &err, &olen);
	  syslog(LOG_INFO, "%s disconnected%s%s", netdev,
		 err ? " : " : "", err ? strerror(err) : "");
	  
	  close(sk);
	  return 0;
	}
    }

  return 0;
}
#endif

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

  fprintf (stderr, "%s\n", batostr (bdaddr));
  fprintf (stderr, "%s\n", batostr (&src_addr));
  
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

struct __service_16 { 
	uint16_t dst;
	uint16_t src;
} __attribute__ ((packed));

struct __service_32 { 
	uint16_t unused1;
	uint16_t dst;
	uint16_t unused2;
	uint16_t src;
} __attribute__ ((packed));

struct __service_128 { 
	uint16_t unused1;
	uint16_t dst;
	uint16_t unused2[8];
	uint16_t src;
	uint16_t unused3[7];
} __attribute__ ((packed));

/* Create BNEP connection 
 * sk      - Connect L2CAP socket
 * role    - Local role
 * service - Remote service
 * dev     - Network device (contains actual dev name on return)
 */
int bnep_create_connection(int sk, uint16_t role, uint16_t svc)
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

	if (send(sk, pkt, sizeof(*req) + sizeof(*s), 0) < 0)
		return -1;

receive:
	/* Get response */
	r = recv(sk, pkt, BNEP_MTU, 0);
	if (r <= 0)
		return -1;

	errno = EPROTO;

	if (r < sizeof(*rsp))
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

  return (struct bt_service *)s;
}

static void
pan_connect (GtkWidget *w, struct bt_service_pan *svc)
{
  int fd;
  char buf[16];

  fd = create_connection (&svc->bd->bdaddr);
  
  bnep_create_connection (fd, BNEP_SVC_PANU, BNEP_SVC_NAP);

  sprintf (buf, "%d", fd);

  if (vfork () == 0)
    {
      execl (PREFIX "/lib/gpe-bluetooth/bnep-helper", PREFIX "/lib/gpe-bluetooth/bnep-helper", buf, NULL);
      perror ("exec");
    }
}

static void
pan_popup_menu (struct bt_service *svc, GtkWidget *menu)
{
  struct bt_service_pan *pan;
  GtkWidget *w;
  
  pan = (struct bt_service_pan *)svc;

  w = gtk_menu_item_new_with_label (_("Connect PAN"));
  g_signal_connect (G_OBJECT (w), "activate", G_CALLBACK (pan_connect), svc);
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
