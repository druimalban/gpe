/*
 * gpe-conf
 *
 * Copyright (C) 2005  Florian Boor <florian@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * GPE keys, keyboard and button module.
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

#include "keys.h"
#include "../applets.h"
#include "keyboard.h"
#include "keyctl.h"
#include "kbd.h"

/* --- local types and constants --- */

/* --- module global variables --- */

/* --- local intelligence --- */

/* --- gpe-conf interface --- */

void
Keys_Free_Objects ()
{
}

void
Keys_Save ()
{
	const char *model, *port;
	char *str;

	keyboard_get_config(&model, &port);
	str = g_strdup_printf("%s %s", model, port);
	suid_exec("KBDD", str);
	g_free(str);
	
	Kbd_Save ();
	Keyctl_Save ();
}

void
Keys_Restore ()
{
}

GtkWidget *
Keys_Build_Objects (void)
{
	GtkWidget *label, *page;
	GtkWidget *notebook;

	notebook = gtk_notebook_new();
	
	page = Keyctl_Build_Objects();
	label = gtk_label_new(_("Buttons"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);
	
	page = Kbd_Build_Objects();
	label = gtk_label_new(_("Virtual Keyboard"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);
	
	page = Keyboard_Build_Objects();
	label = gtk_label_new(_("External Keyboard"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);
	
	return notebook;
}
