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

#define NUM_BUTTONS 5

/* local types */

typedef struct 
{
	char *pixname;
	char *ident;
	char *command;
	int type;
}
t_buttondef;


/* local variables */

static struct
{
	GtkWidget *button[NUM_BUTTONS];
	GdkPixbuf *p;
	GtkWidget *edit;
	GtkWidget *icon;
	GtkWidget *select;
}
self;

t_buttondef buttondef[NUM_BUTTONS] = {
	{NULL, "XF86AudioRecord","gpe-soundbite record --autogenerate-filename $HOME_VOLATILE",0},
    {NULL, "XF86Calendar","gpe-calendar",0},
	{NULL, "telephone","gpe-contacts",0},
	{NULL, "XF86Mail","gpe-taskmanager",0},
	{NULL, "XF86Start","mbcontrol -desktop",0}};

static int active_button = 0;

struct gpe_icon local_icons[] = {
	{ "ipaq" , "ipaq-s1"},
/*	{ "button1", PREFIX "/share/pixmaps/button1.png" }, 
	{ "button2", PREFIX "/share/pixmaps/button2.png" }, 
	{ "button3", PREFIX "/share/pixmaps/button3.png" }, 
	{ "button4", PREFIX "/share/pixmaps/button4.png" }, 
	{ "button5", PREFIX "/share/pixmaps/button5.png" }, 
*/	{ NULL, NULL }
};
	
	
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
on_button_select (GtkButton * button, gpointer user_data)
{
	int nr = (int)user_data;
	if ((nr == active_button) ||
		((active_button < 0) && (active_button >= NUM_BUTTONS)))
		return;	
	
	g_free(buttondef[active_button].command);
	buttondef[active_button].command = 
		g_strdup(gtk_entry_get_text(GTK_ENTRY(self.edit)));
	gtk_entry_set_text(GTK_ENTRY(self.edit),buttondef[nr].command);
#warning pixmap, box, type
	active_button = nr;
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
	GtkWidget *vbox = gtk_vbox_new(FALSE,0);
	GtkWidget *layout1 = gtk_layout_new (NULL, NULL);
	GtkWidget *bDefault = gtk_button_new_with_label (_("Defaults"));
	GtkWidget *scroll =	gtk_scrolled_window_new (NULL,NULL);
	GtkWidget *bFile = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	GtkWidget *table = gtk_table_new(3,2,FALSE);
	int i;
	
	
	gpe_load_icons(local_icons);
	
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);
	self.p = gpe_find_icon ("ipaq");
	self.edit = gtk_entry_new();
	self.icon = gtk_image_new_from_pixbuf(gpe_find_icon("icon"));
	self.select = gtk_combo_new();
	
	gtk_box_pack_start(GTK_BOX(vbox),scroll,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(vbox),table,FALSE,TRUE,0);
	
	gtk_table_attach(GTK_TABLE(table),self.icon,0,1,0,2,GTK_FILL,GTK_EXPAND,0,0);
	gtk_table_attach(GTK_TABLE(table),self.edit,1,2,1,2,GTK_FILL | GTK_EXPAND,GTK_FILL | GTK_EXPAND,0,0);
	gtk_table_attach(GTK_TABLE(table),self.select,1,4,0,1,GTK_FILL | GTK_EXPAND ,GTK_FILL | GTK_EXPAND,0,0);
	gtk_table_attach(GTK_TABLE(table),bFile,3,4,1,2,GTK_FILL ,GTK_FILL ,0,0);
	
	self.button[0] = gtk_radio_button_new(NULL);
	self.button[1] = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(self.button[0]));
	self.button[2] = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(self.button[0]));
	self.button[3] = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(self.button[0]));
	self.button[4] = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(self.button[0]));

	gtk_container_add (GTK_CONTAINER (scroll), layout1);

	gtk_container_set_border_width (GTK_CONTAINER (scroll), 2);

	if (self.p)
	{
		GtkWidget *pixmap1 = gtk_image_new_from_pixbuf (self.p);
		gtk_layout_put (GTK_LAYOUT (layout1), pixmap1, 30, 5);

	}

	gtk_layout_put (GTK_LAYOUT (layout1), self.button[0], 17, 33);
	gtk_layout_put (GTK_LAYOUT (layout1), self.button[1], 44, 150);
	gtk_layout_put (GTK_LAYOUT (layout1), self.button[2], 75, 170);
	gtk_layout_put (GTK_LAYOUT (layout1), self.button[3], 125, 169);
	gtk_layout_put (GTK_LAYOUT (layout1), self.button[4], 175, 152);
	gtk_layout_put (GTK_LAYOUT (layout1), bDefault, 182, 1);

	g_signal_connect_after (G_OBJECT (bFile), "clicked",
				   G_CALLBACK(on_button_clicked),bFile);
	
	g_signal_connect_after (G_OBJECT (self.button[0]), "clicked",
				   G_CALLBACK(on_button_select),(void *)0);
	g_signal_connect_after (G_OBJECT (self.button[1]), "clicked",
				   G_CALLBACK(on_button_select),(void *)1);
	g_signal_connect_after (G_OBJECT (self.button[2]), "clicked",
				   G_CALLBACK(on_button_select),(void *)2);
	g_signal_connect_after (G_OBJECT (self.button[3]), "clicked",
				   G_CALLBACK(on_button_select),(void *)3);
	g_signal_connect_after (G_OBJECT (self.button[4]), "clicked",
				   G_CALLBACK(on_button_select),(void *)4);
	g_signal_connect (G_OBJECT (bDefault), "clicked",
				   G_CALLBACK(on_defaults_clicked), NULL);
	
	gtk_widget_grab_focus(self.button[0]);
	gtk_entry_set_text(GTK_ENTRY(self.edit),buttondef[0].command);
	active_button = 0;
	/* make string variables from initial constants */
	for (i=0;i<NUM_BUTTONS;i++)
		buttondef[i].command = g_strdup(buttondef[i].command);
	
//	init_buttons ();
	
	return vbox;
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
/*
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
*/
	gtk_entry_set_text (GTK_ENTRY (self.edit), file);
}
