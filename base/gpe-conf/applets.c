/*
 * gpe-conf
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
#include <time.h>
#include <libintl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <gtk/gtk.h>
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE		/* Pour GlibC2 */
#endif
#include <time.h>
#include "applets.h"
#include <gpe/errorbox.h>
#include <gpe/pixmaps.h>
#include "suid.h"

/* indicator for opened file selector */
static int selector_open = FALSE;

/* we remember the password */
static gchar *password = NULL;


void
printlog (GtkWidget * textview, gchar * str)
{
	GtkTextBuffer *log;
	log = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
	gtk_text_buffer_insert_at_cursor (GTK_TEXT_BUFFER (log), str, -1);
}


/***************************************************************************************/
/* Return 1 if a file exists & can be read, 0 otherwise.*/

int
file_exists (char *fn)
{
	FILE *inp;

	inp = fopen (fn, "r");
	if (inp)
	{
		fclose (inp);
		return 1;
	}
	return 0;
}

/***************************************************************************************/

GtkWidget *
make_menu_from_dir (char *path, int (*entrytest) (char *path), char *current)
{
	DIR *dir;
	struct dirent *entry;
	GtkWidget *menu = gtk_menu_new ();
	int i = 0, selected = -1;
	dir = opendir (path);
	if (dir)
	{
		while ((entry = readdir (dir)))
		{
			char *temp;
			GtkWidget *menuitem;

			if (entry->d_name[0] == '.')
				continue;

			/* read the file if we don't want to ignore it */
			temp = g_strdup_printf ("%s/%s", path, entry->d_name);
			if (entrytest != 0 && !entrytest (temp))
			{
				g_free (temp);
			}
			else
			{
				menuitem =
					gtk_menu_item_new_with_label (entry->
								      d_name);
				gtk_object_set_data_full (GTK_OBJECT
							  (menuitem),
							  "fullpath", temp,
							  (GtkDestroyNotify)
							  g_free);
				i++;
				gtk_widget_show (menuitem);
				gtk_menu_append (GTK_MENU (menu), menuitem);
				if (current != 0
				    && strcmp (current, temp) == 0)
					selected = i - 1;
			}
		}
		closedir (dir);
	}
	if (selected != -1)
		gtk_menu_set_active (GTK_MENU (menu), selected);
	return menu;
}


/* 
  Returns a list of directory names in a path containing 
  something given by filter.
*/
GList *
make_items_from_dir (char *path, char* filter)
{
	DIR *dir;
	struct dirent *entry;
	GList *items = NULL;
	char *filterpath = NULL;
	dir = opendir (path);
	
	if (dir)
	{
		while ((entry = readdir (dir)))
		{
			if (entry->d_name[0] == '.')
				continue;
			filterpath = g_strdup_printf("%s/%s/%s",path,entry->d_name,filter);
			if (!access(filterpath,F_OK))
				items = g_list_append (items,
					       g_strdup (entry->d_name));
			g_free(filterpath);
		}
		closedir (dir);
	}
	return items;
}

/***************************************************************************************/
int
mystrcmp (char *s, char *c)
{
	int i = 0;
	while (*s == ' ')
	{
		s++;
		i++;
	}
	while (*c)
	{
		if (*s != *c)
			return -1;
		s++;
		c++;
		i++;
	}
	return i;
}

/***************************************************************************************/


// Maybe this should be in libgpewidget
// but, this doesnt fit in the gtk "way of life"

static void
select_fs (GtkButton * button, gpointer user_data)
{
	char *file;
	struct stat st;
	struct fstruct *s = (struct fstruct *) user_data;

	file = g_strdup (gtk_file_selection_get_filename
			 (GTK_FILE_SELECTION (s->fs)));
	if (stat (file, &st) == 0)
	{
		if (S_ISDIR (st.st_mode))
		{
			// TODO dont do anything for now. (work in libgpewidget)
		}
		else
		{
			s->File_Selected (file, s->data);
			gtk_widget_destroy (GTK_WIDGET (s->fs));
		}
	}
	g_free (file);
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
		 void (*File_Selected) (char *filename, gpointer data),
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



/***************************************************************************************/


int
runProg (char *cmd)
{
	int status;
	pid_t pid;

	char *c, *argv[5];
	int argc = 0;
	while (cmd && (*cmd != (char) NULL))
	{
		if ((c = strchr (cmd, ' ')) != NULL)
			*c++ = (char) NULL;
		argv[argc++] = cmd;
		cmd = c;
	}
	argv[argc++] = NULL;

	if ((pid = fork ()) < 0)
	{
		perror ("fork");
		return (1);
	}
	if (pid == 0)
	{
		execvp (*argv, argv);
		perror (*argv);
		return (2);
	}
	while (wait (&status) != pid) /* do nothing */ ;

	return status;
}

int
system_and_gfree (gchar * cmd)
{
	int rv;
	gchar *buf;
	rv = runProg (cmd);
	if (rv != 0)
	{
		buf = g_strdup_printf
			("%s\n failed with return code %d\nperhaps you have \nmisinstalled something",
			 cmd, rv);
		gpe_error_box (buf);
		g_free (buf);
	}
	g_free (cmd);
	return rv;
}

/*
	Check if user is allowed to exec command / change setting.
	If not ask for root password and check.
*/
int
suid_exec (const char *cmd, const char *params)
{
	GtkWidget *label, *dialog, *icon, *passwd_entry;
	GtkWidget *hbox, *btn;
	GdkPixbuf *p;
	gint res = 0;

	struct passwd *pwent;

	if ((password) || (check_user_access (cmd) == TRUE) || no_root_passwd())
	{
		fprintf (suidout, "%s\n%s\n%s\n", cmd, "<none>", params);
		fflush (suidout);
		return 0;
	}
	else
	{
		dialog = gtk_dialog_new_with_buttons (_("Root access"),
						      GTK_WINDOW (mainw),
						      GTK_DIALOG_MODAL |
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_STOCK_CANCEL,
						      GTK_RESPONSE_REJECT,
						      NULL);
		btn = gtk_dialog_add_button(GTK_DIALOG(dialog),
				GTK_STOCK_OK,
				GTK_RESPONSE_ACCEPT);
		GTK_WIDGET_SET_FLAGS (btn, GTK_CAN_DEFAULT);
		gtk_widget_grab_default(btn);

		
		label = gtk_label_new (_ ("Some options are only\n"\
			"accessible for user root.\nPlease enter password."));
		hbox = gtk_hbox_new (FALSE, 4);

		gtk_widget_realize (dialog);

		p = gpe_find_icon ("lock");
		icon = gtk_image_new_from_pixbuf (p);
		gtk_box_pack_start (GTK_BOX (hbox), icon, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 4);

		passwd_entry = gtk_entry_new ();
		gtk_entry_set_activates_default(GTK_ENTRY(passwd_entry), TRUE);

		gtk_entry_set_visibility (GTK_ENTRY (passwd_entry), FALSE);

		gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
				   hbox);
		gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
				   passwd_entry);

		gtk_widget_show_all (dialog);

		pwent = getpwuid (0);

		while (res != GTK_RESPONSE_REJECT)
		{
			res = gtk_dialog_run (GTK_DIALOG (dialog));
			password =
				g_strdup (gtk_entry_get_text
					  (GTK_ENTRY (passwd_entry)));
			if (!strcmp
			    (crypt (password, pwent->pw_passwd),
			     pwent->pw_passwd))
			{
				fprintf (suidout, "%s\n%s\n%s\n", cmd,
					 password, params);
				fflush (suidout);
				gtk_widget_destroy (dialog);
				return 0;
			}
			else
			{
				if (password)
					free (password);
				password = NULL;
				if (res != GTK_RESPONSE_REJECT)
					gpe_error_box (_("Sorry, wrong password."));
			}
		}
		gtk_widget_destroy (dialog);
	}
	return -1;
}
