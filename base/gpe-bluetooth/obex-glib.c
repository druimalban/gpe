/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib.h>
#include <openobex/obex.h>

#include "obex-glib.h"

struct obex_channel
{
  GIOChannel *chan;
  int fd;
  int source;
};

static GList *channels;

static gboolean
io_callback (GIOChannel *source, GIOCondition cond, gpointer data)
{
  obex_t *obex = (obex_t *)data;

  OBEX_HandleInput (obex, 0);

  return TRUE;
}

void
obex_disconnect_input (obex_t *obex)
{
  int fd;
  GList *i;

  fd = OBEX_GetFD (obex);

  for (i = channels; i; i = i->next)
    {
      struct obex_channel *chan = i->data;

      chan = i->data;

      if (chan->fd == fd)
	{
	  channels = g_list_remove (channels, chan);
	  g_source_remove (chan->source);
	  g_io_channel_unref (chan->chan);
	  g_free (chan);
	  return;
	}
    }

  g_critical ("Couldn't find channel in list.\n");
}

void
obex_connect_input (obex_t *obex)
{
  struct obex_channel *chan;

  chan = g_new (struct obex_channel, 1);
  chan->fd = OBEX_GetFD (obex);
  chan->chan = g_io_channel_unix_new (chan->fd);
  chan->source = g_io_add_watch (chan->chan, G_IO_IN | G_IO_ERR | G_IO_HUP, io_callback, obex);
  channels = g_list_prepend (channels, chan);  
}
