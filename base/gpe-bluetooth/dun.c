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
#include <pty.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include "main.h"
#include "sdp.h"
#include "dun.h"

#define _(x) gettext(x)

struct bt_service_dun
{
  struct bt_service service;

  struct bt_device *bd;

  unsigned int port;
};

static struct bt_service_desc dun_service_desc;

static struct bt_service *
dun_scan (sdp_record_t *rec, struct bt_device *bd)
{
  struct bt_service_dun *s;

  s = g_malloc (sizeof (*s));

  s->service.desc = &dun_service_desc;
  s->bd = bd;

  return (struct bt_service *)s;
}

void
dun_init (void)
{
  sdp_uuid16_create (&dun_service_desc.uuid, DIALUP_NET_SVCLASS_ID);

  dun_service_desc.scan = dun_scan;

  service_desc_list = g_slist_prepend (service_desc_list, &dun_service_desc);
}
