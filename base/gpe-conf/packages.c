/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *	             2003  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * GPE configuration package manager module.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include <libintl.h>
#define _(x) gettext(x)

#include <gtk/gtk.h>

#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>

#include "packages.h"
#include "applets.h"
#include "suid.h"


/* --- local types and constants --- */


/* --- module global variables --- */

static GtkWidget *notebook;

/* --- local intelligence --- */


/* --- gpe-conf interface --- */

void
Packages_Free_Objects ()
{
}

void
Packages_Save ()
{

}

void
Packages_Restore ()
{
	
}

GtkWidget *
Packages_Build_Objects (void)
{
  notebook = gtk_notebook_new();
  gtk_container_set_border_width (GTK_CONTAINER (notebook), gpe_get_border ());
  
  return notebook;
}
