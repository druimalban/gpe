/*
 * Copyright (C) 2002 Colin Marquardt <ipaq@marquardt-home.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <unistd.h> /* for access() */

#include <glib.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <gpe/pixmaps.h>
#include <gpe/init.h>


static struct gpe_icon my_icons[] = {
  { "login_bg", PREFIX "/share/pixmaps/gpe-default-bg.png" },
  { NULL, NULL }
};

int 
main (int argc, char *argv[])
{
  GdkPixmap *pixmap;
  GdkBitmap *bitmap;
  
  gpe_application_init (&argc, &argv);

  if (argc > 2) {
    g_error ("gpe-setbg: Expecting at most 1 argument.");
    exit (1);
  }
  if (argc == 2) {
    if (access (argv[1], R_OK) == 0) /* set file if given on the command line */
      my_icons[0].filename = g_strdup (argv[1]);
    else {
      g_print ("gpe-setbg: ERROR: Could not read image file\n'%s'\n", argv[1]);
      exit (1);
    }
  }

#warning FIXME: fail silently here (at least not with a dialog box, ugly at login)
  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);
  
  if (gpe_find_icon_pixmap ("login_bg", &pixmap, &bitmap)) {
    g_print ("gpe-setbg: Setting background.\n");
    gdk_window_set_back_pixmap (GDK_ROOT_PARENT (), pixmap, 0);
  }

  return (0);
}
