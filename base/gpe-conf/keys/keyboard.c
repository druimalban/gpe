/*
 * gpe-conf
 *
 * Copyright (C) 2005  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * GPE external keyboard configuration module.
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

#include "../applets.h"


/* --- local types and constants --- */

/* --- module global variables --- */

/* --- local intelligence --- */

/* --- gpe-conf interface --- */

void
Keyboard_Free_Objects ()
{
}

void
Keyboard_Save ()
{
}

void
Keyboard_Restore ()
{
}

GtkWidget *
Keyboard_Build_Objects (void)
{
	GtkWidget *label;
	GtkWidget *table;
	gchar *buf;
	GtkTooltips *tooltips;

	tooltips = gtk_tooltips_new ();

	buf = malloc (255);

	table = gtk_table_new (3, 2, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table),
					gpe_get_border ());
	gtk_table_set_row_spacings (GTK_TABLE (table), gpe_get_boxspacing ());
	gtk_table_set_col_spacings (GTK_TABLE (table), gpe_get_boxspacing ());

	gtk_object_set_data (GTK_OBJECT (table), "tooltips", tooltips);
	gtk_tooltips_set_tip (tooltips, 
	                      table, 
	                      _("External keyboard configuration."),
	                      NULL);

	label = gtk_label_new (NULL);
	snprintf (buf, 100, "<b>%s</b>", _("External Keyboard Type"));
	gtk_label_set_markup (GTK_LABEL (label), buf);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 2, 0, 1,
			  GTK_FILL, GTK_FILL, 0, 0);

	gtk_tooltips_set_tip (tooltips, label,
			              _("Here you may select your external keyboard model."),
	                      NULL);

	return table;
}
