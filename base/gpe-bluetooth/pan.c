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
#include <glib.h>

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include "main.h"

struct bt_service_pan
{
  struct bt_service svc;
};

static struct bt_service_desc pan_service_desc;

static struct bt_service *
pan_scan (sdp_record_t *rec)
{
  return NULL;
}

void
pan_init (void)
{
  sdp_uuid16_create (&pan_service_desc.uuid, NAP_SVCLASS_ID);  

  pan_service_desc.scan = pan_scan;

  service_desc_list = g_slist_prepend (service_desc_list, &pan_service_desc);
}
