/*
 * Copyright (C) 2004  Nils Faerber <nils.faerber@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <libintl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <gtk/gtk.h>

#include <libintl.h>
#define _(x) gettext(x)

#include "filechooser.h"


/* global and static so we can reuse them */
static GtkWidget *fchooser = NULL;
static GtkFileFilter *filter = NULL;


int package_choose(GtkWidget *parent, void (*File_Selected) (char *filename, gpointer data))
{
GtkResponseType resp;

	if (fchooser == NULL) {
		fchooser = gtk_file_chooser_dialog_new(_("Choose packages"), GTK_WINDOW(parent), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
		gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(fchooser), "/tmp", NULL);
		gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(fchooser), "/var", NULL);
		gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(fchooser), "/", NULL);
		gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(fchooser), FALSE);
	}

	if (filter == NULL) {
		filter = gtk_file_filter_new();
		gtk_file_filter_add_pattern (filter, "*.ipk");
		gtk_file_filter_add_pattern (filter, "*.ipkg");
	}
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(fchooser), filter);

	resp = gtk_dialog_run (GTK_DIALOG (fchooser));

	if (resp == GTK_RESPONSE_OK || resp == GTK_RESPONSE_ACCEPT) {
		char *filename;

		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fchooser));
		File_Selected(filename, NULL);
		g_free (filename);
	}
	gtk_widget_hide (fchooser);

return 0;
}
