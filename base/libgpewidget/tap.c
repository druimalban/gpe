/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <gdk/gdk.h>

struct tap
{
  GdkEvent ev;
  gboolean flag;
};

static struct tap *t;

static gboolean
timeout (gpointer p)
{
  struct tap *tt = (struct tap *)p;

  if (tt->flag == TRUE)
    {
      tt->ev.button.button = 3;
      tt->ev.button.type = GDK_BUTTON_PRESS;
      gtk_main_do_event (&tt->ev);
      tt->ev.button.type = GDK_BUTTON_RELEASE;
      gtk_main_do_event (&tt->ev);
    }

  t = NULL;

  g_free (tt);

  return FALSE;
}

static void
filter (GdkEvent *ev, gpointer data)
{
  if (ev->type == GDK_BUTTON_PRESS 
      && ev->button.button == 1 
      && t == NULL)
    {
      struct tap *tt = g_malloc (sizeof (struct tap));
      g_timeout_add (500, timeout, tt);
      tt->flag = TRUE;
      memcpy (&tt->ev, ev, sizeof (*ev));
      t = tt;
    }
  else if (ev->type == GDK_BUTTON_RELEASE 
	   && ev->button.button == 1 
	   && t 
	   && t->ev.any.window == ev->any.window)
    {
      t->flag = FALSE;
      t = NULL;
    }

  gtk_main_do_event (ev);
}

void
tap_hold_init (void)
{
  gdk_event_handler_set (filter, NULL, NULL);
}
