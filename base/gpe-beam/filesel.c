/*
 * taken from gpe-conf file selection
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *               2003  Florian Boor <florian.boor@kernelconcepts.de>
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

#include "filesel.h"

/* indicator for opened file selector */
static int selector_open = FALSE;


static void
select_fs (GtkButton * button, gpointer user_data)
{
	char *file = NULL;
	struct stat st;
	struct fstruct *s = (struct fstruct *) user_data;
		
	file = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION (s->fs)));
	if (stat (file, &st) == 0)
	{
		if (S_ISDIR (st.st_mode))
		{
			// TODO dont do anything for now. (work in libgpewidget)
		}
		else
		{
			gtk_widget_hide (GTK_WIDGET (s->fs));
			gtk_widget_destroy (GTK_WIDGET (s->fs));
			s->File_Selected (file, s->data);
		}
		free(file);	
	}
	selector_open = FALSE;
}


static void
cancel_fs (GtkButton * button, gpointer user_data)
{
	struct fstruct *s = (struct fstruct *) user_data;

	if (s->Cancel)
		s->Cancel (s->data);

	gtk_widget_destroy (GTK_WIDGET (s->fs));
	selector_open = FALSE;
}


void
freedata (GtkWidget * ignored, gpointer user_data)
{
	struct fstruct *s = (struct fstruct *) user_data;
	free (s);
	selector_open = FALSE;
}


void
ask_user_a_file (char *path, char *prompt,
		 void (*File_Selected) (const gchar *filename, gpointer data),
		 void (*Cancel) (gpointer data), gpointer data)
{
	char buf[1024];
	char *ret = getcwd (buf, 1024);
	
	if (path)		// this is a hack, we're all waiting a gtk_file_selection_change_directory().. (TODO)
		chdir (path);
	{
		GtkWidget *fileselection1 =
			gtk_file_selection_new (prompt ? prompt :
						"Select File");
		GtkWidget *ok_button1 =
			GTK_FILE_SELECTION (fileselection1)->ok_button;
		GtkWidget *cancel_button1 =
			GTK_FILE_SELECTION (fileselection1)->cancel_button;
		struct fstruct *s =
			(struct fstruct *) malloc (sizeof (struct fstruct));

		if (selector_open)
			return;
		selector_open = TRUE;

		if (ret)
			chdir (buf);
		s->fs = fileselection1;
		s->File_Selected = File_Selected;
		s->Cancel = Cancel;
		s->data = data;

		GTK_WINDOW (fileselection1)->type = GTK_WINDOW_TOPLEVEL;

		gtk_widget_show (ok_button1);
		gtk_signal_connect (GTK_OBJECT (ok_button1), "clicked",
				    GTK_SIGNAL_FUNC (select_fs),
				    (gpointer) s);

		gtk_widget_show (cancel_button1);
		gtk_signal_connect (GTK_OBJECT (cancel_button1), "clicked",
				    GTK_SIGNAL_FUNC (cancel_fs),
				    (gpointer) s);

		gtk_signal_connect (GTK_OBJECT (s->fs), "destroy", (GtkSignalFunc) freedata, (gpointer) s);	// in case of destruction by close (X) button

		g_signal_connect_swapped (GTK_OBJECT (ok_button1),
					  "clicked",
					  G_CALLBACK (gtk_widget_destroy),
					  (gpointer) s->fs);

		g_signal_connect_swapped (GTK_OBJECT (cancel_button1),
					  "clicked",
					  G_CALLBACK (gtk_widget_destroy),
					  (gpointer) s->fs);
		gtk_widget_show (s->fs);
	}
}
