/*
 * gpe-conf
 *
 * Copyright (C) 2004  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * GPE sound settings module.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libintl.h>
#define _(x) gettext(x)

#include <gtk/gtk.h>

#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>

#include "sound.h"


void
Sound_Free_Objects ()
{
}

void
Sound_Save ()
{

}

void
Sound_Restore ()
{
}

GtkWidget *
Sound_Build_Objects (void)
{
	GtkWidget *table;
	GtkWidget *tw;
	gchar *ts = NULL;
	
	table = gtk_table_new(6,2,FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table),gpe_get_boxspacing());
	gtk_table_set_col_spacings(GTK_TABLE(table),gpe_get_boxspacing());
	
	tw = gtk_label_new(NULL);
	ts = g_strdup_printf("<b>%s</b>",_("Sound Settings"));
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0,0.5);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table), tw, 0, 2, 0, 1, GTK_FILL | GTK_EXPAND,
                     GTK_FILL, 0, 0);
	
	return table;
}
