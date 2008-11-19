/* gst_bus_timed_pop.c - gst_bus_timed_pop replacement.
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

#include <glib.h>
#include <gst/gst.h>

/* In this sucky reimplementation of gst_bus_timed_pop, we busy wait.
   We could do better, however, we know that our code does not use
   this extensively.  */
GstMessage *
gst_bus_timed_pop (GstBus *bus, GstClockTime timeout)
{
  for (;;)
    {
      GstMessage *msg = gst_bus_pop (bus);
      if (msg)
	return msg;

      if (timeout == 0)
	return NULL;

      /* Wait 200 ms.  */
      int step = G_USEC_PER_SEC / 5;
      g_usleep (step);
      if (timeout / 1000 < step)
	return NULL;

      timeout -= step * 1000;
    }
}
