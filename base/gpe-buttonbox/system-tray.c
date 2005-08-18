/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <libintl.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <locale.h>

#include "config.h"
#include "globals.h"

#define _(_x) gettext (_x)

#define MAX_DATA_BYTES 20

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

static GdkFilterReturn 
tray_opcode (GdkXEvent *xev, GdkEvent *ev)
{
  XClientMessageEvent *c = (XClientMessageEvent *)xev;
  Window w = c->window;

  switch (c->data.l[1])
    {
    case SYSTEM_TRAY_REQUEST_DOCK:
      fprintf (stderr, "window %x requests dock\n", w);
      break;
    }
}

gboolean
system_tray_init (Display *dpy, Window window)
{
  GdkAtom opcode;

  if (XGetSelectionOwner (dpy, atoms[_NET_SYSTEM_TRAY_Sn]) != None)
    return FALSE;

  XSetSelectionOwner (dpy, atoms[_NET_SYSTEM_TRAY_Sn], window, CurrentTime);
  
  opcode = gdk_atom_intern ("_NET_SYSTEM_TRAY_OPCODE", FALSE);
  gdk_add_client_message_filter (opcode, (GdkFilterFunc)tray_opcode, NULL);

  return TRUE;
}
