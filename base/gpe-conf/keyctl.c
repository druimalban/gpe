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
 * GPE keybindings software setup untility (here: keylaunch)
 *
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
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <libintl.h>

#include <time.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>
#include <gpe/errorbox.h>
#include <gpe/question.h>

#include "applets.h"
#include "keyctl.h"

#include <gpe/spacing.h>


char buttons[9][1024];
char *keylaunchrc = NULL;

static struct
{
	GtkWidget *button[8];
	GdkPixbuf *p;
}
self;

char *default_keyctl_conf[] = {
	"key=???Pressed XF86AudioRecord:Record Memo:gpe-soundbite record --autogenerate-filename $HOME_VOLATILE",
	"key=???XF86Calendar:Calendar:gpe-calendar",
	"key=???telephone:Contacts:gpe-contacts",
	"key=???XF86Mail:Running programs:gpe-taskmanager",
	"key=???XF86Start:mbcontrol -desktop",
	"key=???XF86PowerDown:apm --suspend",
	"key=???Held XF86PowerDown:-:bl toggle",
	"key=???Combine XF86Calendar XF86Start:gpe-keylock"
};

void FileSelected (char *file, gpointer data);

void
init_buttons ()
{
	FILE *fd;
	GtkButton *target;
	char buffer[1024];
	char *slash;
	char btext[16];
	int i;

	/* default keylaunchrc in homedir */
	keylaunchrc =
		g_strdup_printf ("%s/.keylaunchrc", getenv ("HOME"));
	
	/* if root or no keylaunchrc, we choose another */
	if (!getuid() || access(keylaunchrc,R_OK))
	{
		g_free(keylaunchrc);
		keylaunchrc = g_strdup ("/etc/keylaunchrc");
	}

	buttons[0][0] = 0;
	
	/* read from configfile and set buttons */
	fd = fopen (keylaunchrc, "r");
	if (fd == NULL)
	{
		/* defaults */
		for (i = 0; i < 8; i++)
		{
			strcpy (buttons[i], default_keyctl_conf[i]);
			slash = strrchr (buttons[i], '/');
			if (slash == NULL)
			{
				slash = strrchr (buttons[i], ':');
			};
			if (slash == NULL)
			{
				// FIXME: compensate broken entry!
				slash = buttons[i];
			}
			else
			{
				slash++;	// select next char after selected position
			}
			strncpy (btext, slash, 15);
			btext[15] = '\x0';
			target = GTK_BUTTON (self.button[i]);
			gtk_button_set_label (GTK_BUTTON (target), btext);
		}
	}
	else
	{
		/* load from configfile */
		for (i = 0; i < 8; i++)
		{
			fgets (buffer, 1023, fd);
			slash = strchr (buffer, '\n');
			if (slash != NULL)
			{
				*slash = '\x0';
			}
			strcpy (buttons[i], buffer);
			slash = strrchr (buttons[i], '/');
			if (slash == NULL)
			{
				slash = strrchr (buttons[i], ':');
			};
			if (slash == NULL)
			{
				// FIXME: compensate broken entry!
				slash = buttons[i];
			}
			else
			{
				slash++;	// select next char after selected position
			}
			strncpy (btext, slash, 15);
			btext[15] = '\x0';
			target = GTK_BUTTON (self.button[i]);
			gtk_button_set_label (GTK_BUTTON (target), btext);
		}
		fclose (fd);
//		strncpy (buttons[5], default_keyctl_conf[5], 1023);
//		strncpy (buttons[6], default_keyctl_conf[6], 1023);
	}
}


void
on_button_clicked (GtkButton * button, gpointer user_data)
{
	ask_user_a_file ("/usr/bin", NULL, FileSelected, NULL, button);
}

void
on_defaults_clicked (GtkButton * button, gpointer user_data)
{
	int i;
	char btext[16];
	char *slash;
	
	if (gpe_question_ask
	    (_("Reset button config to default?"), _("Question"), "question",
	     "!gtk-no", NULL, "!gtk-yes", NULL, NULL))
	{
		for (i = 0; i < 8; i++)
		{
			strcpy (buttons[i], default_keyctl_conf[i]);
			slash = strrchr (buttons[i], '/');
			if (slash == NULL)
			{
				slash = strrchr (buttons[i], ':');
			};
			if (slash == NULL)
			{
				slash = buttons[i];
			}
			else
			{
				slash++;	// select next char after selected position
			}
			strncpy (btext, slash, 15);
			btext[15] = '\x0';
			gtk_button_set_label (GTK_BUTTON (self.button[i]),
					      btext);
		}
	}
}


GtkWidget *
Keyctl_Build_Objects ()
{
	GtkWidget *layout1 = gtk_layout_new (NULL, NULL);
	GtkWidget *button_1 = gtk_button_new_with_label ("Record");
	GtkWidget *button_2 = gtk_button_new_with_label ("Calendar");
	GtkWidget *button_5 = gtk_button_new_with_label ("Menu");
	GtkWidget *button_3 = gtk_button_new_with_label ("Contacts");
	GtkWidget *button_4 = gtk_button_new_with_label ("Q");
	GtkWidget *bDefault = gtk_button_new_with_label (_("Defaults"));
	GtkWidget *scroll =
		gtk_scrolled_window_new (GTK_ADJUSTMENT
					 (gtk_adjustment_new
					  (0, 0, 230, 1, 10, 240)),
					 GTK_ADJUSTMENT
					 (gtk_adjustment_new
					  (0, 0, 220, 1, 10, 240)));
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);
	self.p = gpe_find_icon ("ipaq");
	
	
	self.button[0] = button_1;
	self.button[1] = button_2;
	self.button[2] = button_3;
	self.button[3] = button_4;
	self.button[4] = button_5;
	self.button[5] = gtk_button_new();
	self.button[6] = gtk_button_new();
	self.button[7] = gtk_button_new();

	gtk_container_add (GTK_CONTAINER (scroll), layout1);
	gtk_layout_set_size (GTK_LAYOUT (layout1), 230, 220);

	gtk_container_set_border_width (GTK_CONTAINER (scroll), 2);

	GTK_ADJUSTMENT (GTK_LAYOUT (layout1)->hadjustment)->
		step_increment = 10;
	GTK_ADJUSTMENT (GTK_LAYOUT (layout1)->vadjustment)->
		step_increment = 10;

	if (self.p)
	{
		GtkWidget *pixmap1 = gtk_image_new_from_pixbuf (self.p);
		gtk_layout_put (GTK_LAYOUT (layout1), pixmap1, 30, 5);
		gtk_widget_set_usize (pixmap1, 188, 205);

	}

	gtk_layout_put (GTK_LAYOUT (layout1), button_1, 3, 54);
	gtk_widget_set_usize (button_1, 70, 20);

	gtk_layout_put (GTK_LAYOUT (layout1), button_2, 3, 176);
	gtk_widget_set_usize (button_2, 70, 20);

	gtk_layout_put (GTK_LAYOUT (layout1), button_3, 38, 198);
	gtk_widget_set_usize (button_3, 70, 20);

	gtk_layout_put (GTK_LAYOUT (layout1), button_4, 118, 200);
	gtk_widget_set_usize (button_4, 70, 20);

	gtk_layout_put (GTK_LAYOUT (layout1), button_5, 162, 178);
	gtk_widget_set_usize (button_5, 70, 20);

	gtk_layout_put (GTK_LAYOUT (layout1), bDefault, 164, 3);
	gtk_widget_set_usize (bDefault, 70, 20);

	gtk_signal_connect_object (GTK_OBJECT (button_1), "clicked",
				   GTK_SIGNAL_FUNC
				   (on_button_clicked),
				   GTK_OBJECT (button_1));
	gtk_signal_connect_object (GTK_OBJECT (button_2), "clicked",
				   GTK_SIGNAL_FUNC
				   (on_button_clicked),
				   GTK_OBJECT (button_2));
	gtk_signal_connect_object (GTK_OBJECT (button_5), "clicked",
				   GTK_SIGNAL_FUNC
				   (on_button_clicked),
				   GTK_OBJECT (button_5));
	gtk_signal_connect_object (GTK_OBJECT (button_3), "clicked",
				   GTK_SIGNAL_FUNC
				   (on_button_clicked),
				   GTK_OBJECT (button_3));
	gtk_signal_connect_object (GTK_OBJECT (button_4), "clicked",
				   GTK_SIGNAL_FUNC
				   (on_button_clicked),
				   GTK_OBJECT (button_4));
	gtk_signal_connect_object (GTK_OBJECT (bDefault), "clicked",
				   GTK_SIGNAL_FUNC
				   (on_defaults_clicked), NULL);
	init_buttons ();
	
	return scroll;
}


void Keyctl_Free_Objects ();

void
Keyctl_Save ()
{
	/* save new config, force keyctl to reload, and exit */
	int i;
	FILE *fd;
	
	/* User will write to keylaunch in homedir, root global */	
	if (getuid())
	{
		keylaunchrc =
			g_strdup_printf ("%s/.keylaunchrc", getenv ("HOME"));
	}
	else
		keylaunchrc = g_strdup ("/etc/keylaunchrc");

	fd = fopen (keylaunchrc, "w");
	if (fd == NULL)
	{
		gpe_error_box (_("ERROR: Can't open keylaunchrc for writing!\n"));
		return;
	}
	for (i = 0; i < 8; i++)
	{
#ifdef DEBUG
		printf ("button #%d => %s\n", i, buttons[i]);
#endif
		fprintf (fd, "%s\n", buttons[i]);
	}
	fclose (fd);

}

void
Keyctl_Restore ()
{
}


void
FileSelected (char *file, gpointer data)
{
	GtkButton *target = data;
	char btext[16];
	int len;
	char *slash;
	int button_nr;

	if (!strlen (file))
		return;

	for (button_nr = 0;
	     button_nr < 5
	     && self.button[button_nr] != GTK_WIDGET (target); button_nr++) ;
	slash = strrchr (buttons[button_nr], ':') + 1;
	strncpy (slash, file,
		 1023 - (strlen (buttons[button_nr]) - strlen (slash)) - 1);
	buttons[button_nr][1023] = '\x0';

#ifdef DEBUG
	printf ("button %d changed to %s\n", button_nr, file);
#endif
	len = strlen (file);
	slash = strrchr (file, '/');
	if (slash == NULL)
	{
		slash = strrchr (file, ':');
	};
	if (slash == NULL)
	{
		slash = file;
	}
	else
	{
		slash++;	// select next char after selected position
	}
	strncpy (btext, slash, 15);
	btext[15] = '\x0';
#ifdef DEBUG
	printf ("setting label to %s\n", btext);
#endif
	gtk_button_set_label (GTK_BUTTON (target), btext);
}
