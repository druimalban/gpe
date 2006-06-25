/*
 * gpe-conf
 *
 * Copyright (C) 2002   Moray Allan <moray@sermisy.org>,Pierre TARDY <tardyp@free.fr>
 *               2003 - 2006   Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <libintl.h>

#include <gtk/gtk.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include "rotation.h"
#include "gpe/errorbox.h"
#include "../applets.h"

#ifndef DISABLE_XRANDR

extern gchar *RotationLabels;
static Display *dpy = NULL;
static gint screen;

static gchar *xmodmaps[4]=
  {
    "/etc/X11/xmodmap-portrait",
    "/etc/X11/xmodmap-left",
    "/etc/X11/xmodmap-invert",
    "/etc/X11/xmodmap-right"
  };

static gint
xrr_supported (void)
{
  int xrr_event_base, xrr_error_base;
  int xrr_major, xrr_minor;

  if (XRRQueryExtension (dpy, &xrr_event_base, &xrr_error_base) == False
      || XRRQueryVersion (dpy, &xrr_major, &xrr_minor) == 0
      || xrr_major != 1
      || xrr_minor < 1)
    return 0;

  return 1;
}


gint
check_init_rotation(void)
{
	dpy = XOpenDisplay (NULL);
	if (dpy == NULL)
 	{
		fprintf (stderr, "Couldn't open display\n");
		return FALSE;
	}
	screen = DefaultScreen (dpy);
	
	return xrr_supported ();	
}
  

gint 
get_rotation (void)
{
	gint rotation;
	XRRScreenConfiguration *rr_screen;
	Rotation current_rotation;
	
	if (dpy == NULL) 
		if (!check_init_rotation()) return 0;

	rr_screen = XRRGetScreenInfo (dpy, RootWindow (dpy, screen));
	XRRRotations (dpy, screen, &current_rotation);
	XRRFreeScreenConfigInfo (rr_screen);

	switch (current_rotation)
	{
	case RR_Rotate_270:
		rotation = 3;
	break;
	case RR_Rotate_180:
		rotation = 2;
	break;
	case RR_Rotate_90:
		rotation = 1;
	break;
	case RR_Rotate_0:
		rotation = 0;
	break;
	default:
		fprintf (stderr, "Unknown RANDR rotation: %d\n", current_rotation);
		rotation = 0;
	break;
	}
	
  return rotation;
}


void 
set_rotation (gint rotation)
{
	Rotation sc_rotation;
	XRRScreenConfiguration *scr_config;
	Rotation current_rotation;
	gint size;

	if (dpy == NULL) 
		if (!check_init_rotation()) return;
	
	scr_config =  XRRGetScreenInfo (dpy,RootWindow (dpy, screen));
	size = XRRConfigCurrentConfiguration (scr_config, &current_rotation);	

	switch (rotation)
	{
	case 3:
		sc_rotation = RR_Rotate_270;
	break;
	case 2:
		sc_rotation = RR_Rotate_180;
	break;
	case 1:
		sc_rotation = RR_Rotate_90;
	break;
	case 0:
		sc_rotation = RR_Rotate_0;
	break;
	default:
		fprintf (stderr, "Unknown RANDR rotation: %d\n", rotation);
		sc_rotation = RR_Rotate_0;
	break;
	}
	XRRSetScreenConfig (dpy, 
			   scr_config,
			   RootWindow (dpy, screen),
			   size,
			   sc_rotation,
			   CurrentTime);
	
	XRRFreeScreenConfigInfo (scr_config);
	
	/* if maps are provided rotate keypad as well */
	if (!access (xmodmaps[rotation], R_OK))
		system_printf("xmodmap %s", xmodmaps[rotation]);
}
#endif
