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
#include <gtk/gtk.h>

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include "main.h"

extern int bnep_init (void);

struct bt_service_pan
{
  struct bt_service service;

  struct bt_device *bd;
};

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
pan_popup_menu (struct bt_service *svc, GtkWidget *menu)
{
  struct bt_service_pan *pan = (struct bt_service_pan *)svc;
}

void
pan_init (void)
{
  sdp_uuid16_create (&pan_service_desc.uuid, NAP_SVCLASS_ID);  

  pan_service_desc.scan = pan_scan;
  pan_service_desc.popup_menu = pan_popup_menu;

  service_desc_list = g_slist_prepend (service_desc_list, &pan_service_desc);

  if (bnep_init ())
    exit (1);
}
