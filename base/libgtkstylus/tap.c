/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#define THRESHOLD 4

struct tap
{
  GdkDisplay *gdisplay;
  GdkEvent ev;
  gboolean flag;
  gint x, y;
};

static struct tap *t;
static int do_fn;

static gboolean
timeout (gpointer p)
{
  struct tap *tt = (struct tap *)p;

  if (tt->flag == TRUE)
    {
      gint x, y;
      gdk_display_get_pointer (tt->gdisplay, NULL, &x, &y, NULL);
      if ((abs (x - tt->x) < THRESHOLD)
	  && (abs (y - tt->y) < THRESHOLD))
	{
	  tt->ev.button.button = 3;
	  tt->ev.button.type = GDK_BUTTON_PRESS;
	  gtk_main_do_event (&tt->ev);
	  tt->ev.button.type = GDK_BUTTON_RELEASE;
	  gtk_main_do_event (&tt->ev);
	}
    }

  t = NULL;

  g_free (tt);

  return FALSE;
}

static void
filter (GdkEvent *ev, gpointer data)
{
  if (do_fn
      && (ev->button.state & GDK_MOD1_MASK)
      && ev->button.button == 1)
    {
      ev->button.button = 3;
      gtk_main_do_event (ev);
      return;
    }

  if (ev->type == GDK_BUTTON_PRESS 
      && ev->button.button == 1 
      && t == NULL)
    {
      struct tap *tt = g_malloc (sizeof (struct tap));
      g_timeout_add (500, timeout, tt);
      tt->flag = TRUE;
      memcpy (&tt->ev, ev, sizeof (*ev));
      t = tt;
      tt->gdisplay = gdk_x11_lookup_xdisplay (GDK_WINDOW_XDISPLAY (ev->any.window));
      gdk_display_get_pointer (tt->gdisplay, NULL, &tt->x, &tt->y, NULL);
    }
  else if (ev->type == GDK_BUTTON_RELEASE 
	   && ev->button.button == 1 
	   && t)
    {
      t->flag = FALSE;
      t = NULL;
    }

  gtk_main_do_event (ev);
}

void
gtk_module_init (gint *argc, gchar ***argv)
{
  if (getenv ("GTK_STYLUS_DO_FN"))
    do_fn = 1;

  gdk_event_handler_set (filter, NULL, NULL);
}
