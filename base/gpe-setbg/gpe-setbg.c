/*
 * Copyright (C) 2002, 2003 Colin Marquardt <ipaq@marquardt-home.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h> /* for exit() */
#include <unistd.h> /* for access() */

#include <glib.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <gpe/pixmaps.h>
#include <gpe/init.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

static struct gpe_icon my_icons[] = {
  { "login_bg", PREFIX "/share/pixmaps/gpe-default-g.png" },
  { NULL, NULL }
};

static void
set_xrootpmap_id (Pixmap p)
{
  Display *dpy = GDK_DISPLAY ();
  Atom atom = XInternAtom (dpy, "_XROOTPMAP_ID", False);
  Window root = RootWindow (dpy, 0);
  XSetCloseDownMode (dpy, RetainPermanent);
  XChangeProperty (dpy, root, atom, XA_PIXMAP, 32, PropModeReplace, (char *)&p, 1);
  XSync (dpy, False);
}

int 
main (int argc, char *argv[])
{
  GdkPixmap *pixmap;
  GdkBitmap *bitmap;
  
  gpe_application_init (&argc, &argv);

  if (argc > 2) {
    fprintf (stderr, "%s: Expecting at most 1 argument.\n", argv[0]);
    fprintf (stderr, "Usage: %s [gpe-icon-file]\n", argv[0]);
    exit (1);
  }
  if (argc == 2) {
    if (access (argv[1], R_OK) == 0) /* set file if given on the command line */
      my_icons[0].filename = g_strdup (argv[1]);
    else {
      g_print ("%s: ERROR: Could not read image file\n'%s'\n", argv[0], argv[1]);
      exit (1);
    }
  }

#warning FIXME: fail silently here (at least not with a dialog box, ugly at login)
  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);
  
  if (gpe_find_icon_pixmap ("login_bg", &pixmap, &bitmap)) {
    g_print ("%s: Setting background to pixmap %08x.\n",
	     argv[0], (unsigned int)GDK_PIXMAP_XID (pixmap));
    gdk_window_set_back_pixmap (GDK_ROOT_PARENT (), pixmap, 0);
    set_xrootpmap_id (GDK_PIXMAP_XID (pixmap));
  }

  return (0);
}
