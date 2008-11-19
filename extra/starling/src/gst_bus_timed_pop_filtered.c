/* gst_bus_timed_pop_filtered.c - gst_bus_timed_pop_filtered replacement.
   Copyright (C) 2008 Neal H. Walfield <neal@walfield.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see
   <http://www.gnu.org/licenses/>.  */

#define _GNU_SOURCE

#include <stdint.h>
#include <sys/time.h>
#include <gst/gst.h>

static inline uint64_t
now (void)
{
  struct timeval t;
  struct timezone tz;

  if (gettimeofday (&t, &tz) == -1)
    return 0;
  return (t.tv_sec * 1000000ULL + t.tv_usec);
}

/* From the documentation:

   "Get a message from the bus whose type matches the message type
   mask types, waiting up to the specified timeout (and discarding any
   messages that do not match the mask provided).

   If timeout is 0, this function behaves like
   gst_bus_pop_filtered(). If timeout is GST_CLOCK_TIME_NONE, this
   function will block forever until a matching message was posted on
   the bus."  */
GstMessage *
gst_bus_timed_pop_filtered (GstBus *bus,
			    GstClockTime timeout, GstMessageType types)
{
  uint64_t start = now ();
  for (;;)
    {
      GstMessage *msg = gst_bus_timed_pop (bus, timeout);
      if (! msg)
	return NULL;

      if ((GST_MESSAGE_TYPE (msg) & types))
	/* Got one!  */
	return msg;

      /* We're not interested in this message.  */
      gst_message_unref (msg);
  
      if (timeout == 0)
	{
	  if (msg)
	    /* There may be something else on the bus that
	       matches.  */
	    continue;

	  /* There are no messages on the bus.  Don't wait.  */
	  return NULL;
	}

      if (timeout == GST_CLOCK_TIME_NONE)
	/* Wait forever.  */
	continue;

      uint64_t n = now ();
      uint64_t delta = n - start;
      if (delta > timeout / 1000)
	/* Timed out.  */
	return NULL;

      start = n;
      timeout -= delta * 1000;
    }
}
