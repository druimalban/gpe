/*
 * Copyright (C) 2004, 2006 Philip Blundell <philb@gnu.org>
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
#include <string.h>
#include <sys/socket.h>

#include <gtk/gtk.h>
#include <gpe/errorbox.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include <openobex/obex.h>

#include "obexserver.h"
#include "obex-glib.h"
#include "main.h"

#define OBEX_PUSH_HANDLE	10

#define _(x)  (x)

static void
file_received (gchar *name, const uint8_t *data, size_t data_len)
{
  gdk_threads_enter ();

  if (name)
    {
      /* Try to guess type of file */
      gchar *lname;
      lname = g_ascii_strdown (name, -1);
      
      if (strstr (lname, ".vcf"))
	import_vcard (data, data_len);
      else if (strstr (lname, ".vcs"))
	import_vcal (data, data_len);
      else
	import_unknown (name, data, data_len);
      
      g_free (lname);
    }

  gdk_flush ();
  gdk_threads_leave ();
}

static void 
put_done (obex_t *handle, obex_object_t *object)
{
  obex_headerdata_t hv;
  uint8_t hi;
  int hlen;

  const uint8_t *body = NULL;
  int body_len = 0;
  char *name = NULL;

  while (OBEX_ObjectGetNextHeader (handle, object, &hi, &hv, &hlen))	
    {
      switch(hi)	
	{
	case OBEX_HDR_BODY:
	  body = hv.bs;
	  body_len = hlen;
	  break;

	case OBEX_HDR_NAME:
	  name = g_malloc ((hlen / 2) + 1);
	  OBEX_UnicodeToChar (name, hv.bs, hlen);
	  break;
	  
	default:
	  break;
	}
    }

  if (body)
    file_received (name, body, body_len);
  
  if (name)
    g_free (name);
}

static void 
handle_request (obex_t *handle, obex_object_t *object, int event, int cmd)
{
  switch(cmd)	
    {
    case OBEX_CMD_SETPATH:
      OBEX_ObjectSetRsp (object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
      break;
    case OBEX_CMD_PUT:
      OBEX_ObjectSetRsp (object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
      put_done (handle, object);
      break;
    case OBEX_CMD_CONNECT:
      OBEX_ObjectSetRsp (object, OBEX_RSP_SUCCESS, OBEX_RSP_SUCCESS);
      break;
    case OBEX_CMD_DISCONNECT:
      OBEX_ObjectSetRsp (object, OBEX_RSP_SUCCESS, OBEX_RSP_SUCCESS);
      break;
    default:
      fprintf (stderr, "Denied %02x request\n", cmd);
      OBEX_ObjectSetRsp (object, OBEX_RSP_NOT_IMPLEMENTED, OBEX_RSP_NOT_IMPLEMENTED);
      break;
    }
}

static void
obex_conn_event (obex_t *handle, obex_object_t *object, int mode, int event, int obex_cmd, int obex_rsp)
{
  switch (event)
    {
    case OBEX_EV_REQHINT:
      switch(obex_cmd) 
	{
	case OBEX_CMD_PUT:
	case OBEX_CMD_CONNECT:
	case OBEX_CMD_DISCONNECT:
	  OBEX_ObjectSetRsp (object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
	  break;
	default:
	  OBEX_ObjectSetRsp (object, OBEX_RSP_NOT_IMPLEMENTED, OBEX_RSP_NOT_IMPLEMENTED);
	  break;
	}
      break;

    case OBEX_EV_REQ:
      /* Comes when a server-request has been received. */
      handle_request (handle, object, event, obex_cmd);
      break;

    case OBEX_EV_LINKERR:
      obex_disconnect_input (handle);
      break;
    }
}

static void
obex_event (obex_t *handle, obex_object_t *object, int mode, int event, int obex_cmd, int obex_rsp)
{
  obex_t *obex;

  switch (event)
    {
    case OBEX_EV_ACCEPTHINT:
      obex = OBEX_ServerAccept (handle, obex_conn_event, NULL);
      obex_connect_input (obex);
      break;

    default:
      printf ("unknown obex server event %d\n", event);
    }
}

static gboolean
add_opush (sdp_session_t *session, uint8_t chan)
{
  sdp_list_t *svclass_id, *pfseq, *apseq, *root;
  uuid_t root_uuid, opush_uuid, l2cap_uuid, rfcomm_uuid, obex_uuid;
  sdp_profile_desc_t profile[1];
  sdp_list_t *aproto, *proto[3];
  sdp_record_t record;
  sdp_data_t *channel;
  uint8_t formats[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
  //uint8_t formats[] = { 0xff };
  void *dtds[sizeof(formats)], *values[sizeof(formats)];
  int i;
  uint8_t dtd = SDP_UINT8;
  sdp_data_t *sflist;
  gboolean ret = TRUE;
  
  memset((void *)&record, 0, sizeof(sdp_record_t));
  record.handle = 0xffffffff;
  sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
  root = sdp_list_append(0, &root_uuid);
  sdp_set_browse_groups(&record, root);
  
  sdp_uuid16_create(&opush_uuid, OBEX_OBJPUSH_SVCLASS_ID);
  svclass_id = sdp_list_append(0, &opush_uuid);
  sdp_set_service_classes(&record, svclass_id);
  
  sdp_uuid16_create(&profile[0].uuid, OBEX_OBJPUSH_PROFILE_ID);
  profile[0].version = 0x0100;
  pfseq = sdp_list_append(0, profile);
  sdp_set_profile_descs(&record, pfseq);
  
  sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
  proto[0] = sdp_list_append(0, &l2cap_uuid);
  apseq = sdp_list_append(0, proto[0]);
  
  sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
  proto[1] = sdp_list_append(0, &rfcomm_uuid);
  channel = sdp_data_alloc(SDP_UINT8, &chan);
  proto[1] = sdp_list_append(proto[1], channel);
  apseq = sdp_list_append(apseq, proto[1]);
  
  sdp_uuid16_create(&obex_uuid, OBEX_UUID);
  proto[2] = sdp_list_append(0, &obex_uuid);
  apseq = sdp_list_append(apseq, proto[2]);
  
  aproto = sdp_list_append(0, apseq);
  sdp_set_access_protos(&record, aproto);
  
  for (i = 0; i < sizeof(formats); i++) 
    {
      dtds[i] = &dtd;
      values[i] = &formats[i];
    }
  sflist = sdp_seq_alloc(dtds, values, sizeof(formats));
  sdp_attr_add(&record, SDP_ATTR_SUPPORTED_FORMATS_LIST, sflist);
  
  sdp_set_info_attr(&record, "OBEX Object Push", 0, 0);
  
  if (sdp_record_register(session, &record, 0) < 0)
    ret = FALSE;

  sdp_data_free(channel);
  sdp_list_free(proto[0], 0);
  sdp_list_free(proto[1], 0);
  sdp_list_free(proto[2], 0);
  sdp_list_free(apseq, 0);
  sdp_list_free(aproto, 0);

  return ret;
}

static int 
add_service (void)
{
  if (!sdp_session)
    return FALSE;
  
  return add_opush (sdp_session, 10);
}

int
obex_init (void)
{
  obex_t *obex;
  
  obex = OBEX_Init (OBEX_TRANS_BLUETOOTH, obex_event, OBEX_FL_KEEPSERVER);
  if (!obex)
    return FALSE;

  BtOBEX_ServerRegister (obex, NULL, OBEX_PUSH_HANDLE);

  obex_connect_input (obex);
  
  if (add_service () == FALSE)
    {
      gpe_error_box (_("Couldn't register OBEX service"));
      return FALSE;
    }

  return TRUE;
}
