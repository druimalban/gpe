/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
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

#include <openobex/obex.h>

#define OBEX_PUSH_HANDLE	10

struct chan
{
  GIOChannel *chan;
  int fd;
  int source;
};

GList *channels;

static void
file_received (gchar *name, const uint8_t *data, size_t data_len)
{
  fprintf (stderr, "received file %s: %d bytes\n", name, data_len);
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
	  name = g_malloc (hlen / 2);
	  OBEX_UnicodeToChar (name, hv.bs, hlen);
	  break;
	  
	default:
	  fprintf (stderr, "Skipped header %02x\n", hi);
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
handle_disconnect (obex_t *handle)
{
  int fd;
  GList *i;

  fd = OBEX_GetFD (handle);

  for (i = channels; i; i = i->next)
    {
      struct chan *chan = i->data;

      chan = i->data;

      if (chan->fd == fd)
	{
	  channels = g_list_remove (channels, chan);

	  g_source_remove (chan->source);

	  g_io_channel_unref (chan->chan);

	  g_free (chan);
	}
    }
}

static gboolean
io_callback (GIOChannel *source, GIOCondition cond, gpointer data)
{
  obex_t *obex = (obex_t *)data;

  OBEX_HandleInput (obex, 0);

  return TRUE;
}

void
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
      handle_disconnect (handle);
      break;
    }
}

void
obex_event (obex_t *handle, obex_object_t *object, int mode, int event, int obex_cmd, int obex_rsp)
{
  obex_t *obex;
  struct chan *chan;

  switch (event)
    {
    case OBEX_EV_ACCEPTHINT:
      chan = g_malloc (sizeof (*chan));
      obex = OBEX_ServerAccept (handle, obex_conn_event, NULL);
      chan->chan = g_io_channel_unix_new (OBEX_GetFD (obex));
      chan->source = g_io_add_watch (chan->chan, G_IO_IN | G_IO_ERR | G_IO_HUP, io_callback, obex);
      channels = g_list_prepend (channels, chan);
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
  sdp_data_free(channel);
  sdp_list_free(proto[0], 0);
  sdp_list_free(proto[1], 0);
  sdp_list_free(proto[2], 0);
  sdp_list_free(apseq, 0);
  sdp_list_free(aproto, 0);
  
  for (i = 0; i < sizeof(formats); i++) 
    {
      dtds[i] = &dtd;
      values[i] = &formats[i];
    }
  sflist = sdp_seq_alloc(dtds, values, sizeof(formats));
  sdp_attr_add(&record, SDP_ATTR_SUPPORTED_FORMATS_LIST, sflist);
  
  sdp_set_info_attr(&record, "OBEX Object Push", 0, 0);
  
  if (0 > sdp_record_register(session, &record, SDP_RECORD_PERSIST)) 
    {
      printf("Service Record registration failed.\n");
      return FALSE;
    }
  return TRUE;
}

int 
add_service (void)
{
  sdp_session_t *sess;
  bdaddr_t interface;

  sess = sdp_connect (&interface, BDADDR_LOCAL, 0);

  if (!sess)
    return FALSE;

  if (add_opush (sess, 10) == FALSE)
    {
      sdp_close(sess);
      return FALSE;
    }

  sdp_close(sess);
  return TRUE;
}

int
obex_init (void)
{
  obex_t *obex;
  GIOChannel *chan;
  
  obex = OBEX_Init (OBEX_TRANS_BLUETOOTH, obex_event, OBEX_FL_KEEPSERVER);
  if (!obex)
    return FALSE;

  BtOBEX_ServerRegister (obex, NULL, OBEX_PUSH_HANDLE);
  
  chan = g_io_channel_unix_new (OBEX_GetFD (obex));

  g_io_add_watch (chan, G_IO_IN | G_IO_ERR | G_IO_HUP, io_callback, obex);

  add_service ();

  return TRUE;
}
