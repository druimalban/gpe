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
#include <libintl.h>
#include <locale.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>

#include <glib.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include "main.h"
#include "sdp.h"

#define _(x) gettext(x)

gboolean
sdp_find_rfcomm (sdp_record_t *svcrec, int *port)
{
  sdp_list_t *list;
  uuid_t rfcomm_uuid;

  sdp_uuid16_create (&rfcomm_uuid, RFCOMM_UUID);

  if (sdp_get_access_protos (svcrec, &list) == 0) 
    {
      sdp_list_t *next;

      for (; list; list = next)
	{
	  gboolean rfcomm = FALSE;
	  sdp_list_t *protos = (sdp_list_t *)list->data, *nextp;
	  for (; protos; protos = nextp)
	    {
	      sdp_data_t *p = protos->data;

	      switch (p->dtd)
		{
		case SDP_UUID16:
		case SDP_UUID32:
		case SDP_UUID128:
		  if (sdp_uuid_cmp (&p->val.uuid, &rfcomm_uuid) == 0)
		    rfcomm = TRUE;
		  break;
		  
		case SDP_UINT8:
		  if (rfcomm)
		    {
		      *port = p->val.uint8;
		      return TRUE;
		    }
		  break;
		}

	      nextp = protos->next;
	      free (protos);
	    }
	  
	  next = list->next;
	  free (list);
	}
    }

  return FALSE;
}

gboolean
sdp_browse_device (struct bt_device *bd, uint16_t group_id)
{
  sdp_list_t *attrid, *search, *seq, *next;
  uint32_t range = 0x0000ffff;
  int status = -1;
  uuid_t group;
  bdaddr_t bdaddr;
  char str[20];
  sdp_session_t *sess;

  baswap (&bdaddr, &bd->bdaddr);

  sdp_uuid16_create (&group, group_id);

  attrid = sdp_list_append (0, &range);
  search = sdp_list_append (0, &group);
  sess = sdp_connect (BDADDR_ANY, &bdaddr, 0);
  if (!sess) 
    {
      ba2str (&bdaddr, str);
      fprintf (stderr, "Failed to connect to SDP server on %s\n", str);
      return FALSE;
    }
  status = sdp_service_search_attr_req (sess, search, SDP_ATTR_REQ_RANGE, attrid, &seq);

  if (status) 
    return FALSE;

  for (; seq; seq = next)
    {
      sdp_record_t *svcrec = (sdp_record_t *) seq->data;
      sdp_list_t *list = 0;
      bt_service_type type = BT_UNKNOWN;
      uuid_t sub_group;

      if (sdp_get_service_classes (svcrec, &list) == 0) 
	{
	  sdp_list_t *next;

	  for (; list; list = next)
	    {
	      uuid_t *u = list->data;

#if 0
	      if (sdp_uuid_cmp (u, &lap_uuid) == 0)
		type = BT_LAP;
	      else if (sdp_uuid_cmp (u, &dun_uuid) == 0)
		type = BT_DUN;
	      else if (sdp_uuid_cmp (u, &nap_uuid) == 0)
		type = BT_NAP;
#endif

	      next = list->next;
	      free (list);
	    }
	}

      if (type != BT_UNKNOWN)
	{
	  struct bt_service *sv = g_malloc0 (sizeof (struct bt_service));

	  sv->type = type;

	  bd->services = g_slist_append (bd->services, sv);
	}

      if (sdp_get_group_id (svcrec, &sub_group) != -1)
	sdp_browse_device (bd, sub_group.value.uuid16);

      next = seq->next;
      free (seq);
      sdp_record_free (svcrec);
    }
    
  sdp_close (sess);

  return TRUE;
}
