/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *	             2003, 2005  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * GPE keybindings software setup utility (here: keylaunch)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <libintl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/question.h>
#include <gpe/spacing.h>

#include "applets.h"
#include "keyctl.h"

static char *keylaunchrc = NULL;
static char *bg_image = NULL;
static int bg_x = 0, bg_y = 0;
static gboolean menuselect = FALSE;
static int NUM_COMMANDS = 0;
static int NUM_BUTTONS = 0;

#define KEYLAUNCH_BIN   PREFIX "/bin/keylaunch"
#define FILE_COMMANDS   "/etc/gpe/key-commands"
#define FILE_LAYOUT     "/etc/gpe/key-layout"
#define ICON_PATH 		PREFIX "/share/pixmaps"

/* local types */

typedef struct 
{
	char *symbol;
	char *key;
	char *modificator;
	char *command;
	char *title;
	int type;
	int x, y;
}
t_buttondef;

typedef struct
{
	char *title;
	char *command;
}
t_scommand;

/* local variables */

static struct
{
	GtkWidget **buttons;
	GdkPixbuf *p;
	GtkWidget *edit;
	GtkWidget *icon;
	GtkWidget *select;
}
self;

t_scommand  *commands  = NULL;
t_buttondef *buttondefs = NULL;
static int active_button = 0;

static gboolean
commands_load(void)
{
	GKeyFile *cmdfile;
	GError *err = NULL;
	gchar **keys;
	int i, cmdcnt;
	
	cmdfile = g_key_file_new();
	
	if (!g_key_file_load_from_file (cmdfile, FILE_COMMANDS,
                                    G_KEY_FILE_KEEP_COMMENTS, &err))
	{
		gpe_error_box(err->message);
		g_error_free(err);
		return FALSE;
	}
	
	keys = g_key_file_get_keys (cmdfile, "Commands",  &cmdcnt, &err);
	if (keys)
	{
		for (i=0; i < cmdcnt; i++)
		{
			int cnt = 0;
			gchar **vals;
			commands = realloc(commands, (NUM_COMMANDS+1) * sizeof(t_scommand));
			vals = g_key_file_get_string_list (cmdfile, "Commands",
                                               keys[i], &cnt, &err);
			if (cnt == 2)
			{
				commands[NUM_COMMANDS].title = vals[0];
				commands[NUM_COMMANDS].command = vals[1];
				NUM_COMMANDS++;
			}
			else if (cnt == 1)
			{
				commands[NUM_COMMANDS].title = vals[0];
				commands[NUM_COMMANDS].command = "";
				NUM_COMMANDS++;
			}
			else
			{
				if (vals)
					g_strfreev(vals);
			}
		}
		g_strfreev(keys);
	}
	return TRUE;
}
	
static gboolean
layout_load(void)
{
	GKeyFile *layoutfile;
	GError *err = NULL;
	gchar **btndefs;
	int i, btncnt;
	
	layoutfile = g_key_file_new();
	
	if (!g_key_file_load_from_file (layoutfile, FILE_LAYOUT,
                                    G_KEY_FILE_KEEP_COMMENTS, &err))
	{
		gpe_error_box(err->message);
		g_error_free(err);
		return FALSE;
	}
	
	if (g_key_file_has_group(layoutfile, "Global"))
	{
		bg_image = g_key_file_get_string(layoutfile, "Global", "image", &err);
		if (err)
			g_error_free(err);
		bg_x = g_key_file_get_integer(layoutfile, "Global", "offset_x", &err);
		if (err)
			g_error_free(err);
		bg_y = g_key_file_get_integer(layoutfile, "Global", "offset_y", &err);
		if (err)
			g_error_free(err);
	}
	else
	{
		gpe_error_box(_("Button layout file is missing a global section."));
		return FALSE;
	}
	
	btndefs = g_key_file_get_groups(layoutfile, &btncnt);
	if (btndefs)
	{
		for (i=0; i < btncnt; i++)
		{
			if (!strcmp(btndefs[i], "Global")) /* Ignore global section */
				continue;
			buttondefs = realloc(buttondefs, (NUM_BUTTONS+1) * sizeof(t_buttondef));
			buttondefs[NUM_BUTTONS].title = 
				g_key_file_get_string(layoutfile, btndefs[i], "title", &err);
			if (err)
			{
				g_error_free(err);
				continue;
			}
			buttondefs[NUM_BUTTONS].symbol = 
				g_key_file_get_string(layoutfile, btndefs[i], "symbol", &err);
			if (err)
			{
				g_error_free(err);
				continue;
			}
			buttondefs[NUM_BUTTONS].key = 
				g_key_file_get_string(layoutfile, btndefs[i], "key", &err);
			if (err)
			{
				g_error_free(err);
				continue;
			}
			buttondefs[NUM_BUTTONS].modificator = 
				g_key_file_get_string(layoutfile, btndefs[i], "modificator", &err);
			if (err)
			{
				g_error_free(err);
				continue;
			}
			buttondefs[NUM_BUTTONS].command = 
				g_key_file_get_string(layoutfile, btndefs[i], "command", &err);
			if (err)
			{
				g_error_free(err);
				continue;
			}
			buttondefs[NUM_BUTTONS].x = 
				g_key_file_get_integer(layoutfile, btndefs[i], "xpos", &err);
			if (err)
			{
				g_error_free(err);
				continue;
			}
			buttondefs[NUM_BUTTONS].y = 
				g_key_file_get_integer(layoutfile, btndefs[i], "ypos", &err);
			if (err)
			{
				g_error_free(err);
				continue;
			}

			NUM_BUTTONS++;
		}
		g_strfreev(btndefs);
	}
		
	return TRUE;
}


void FileSelected (char *file, gpointer data);


void
init_buttons ()
{
	FILE *fd;
	char *buffer = NULL;
	char *slash;
	int i;
	size_t len;
#warning todo correlate read settings with defined buttons, support combinations
	active_button = 0;
	
	/* default keylaunchrc in homedir */
	keylaunchrc =
		g_strdup_printf ("%s/.keylaunchrc", getenv ("HOME"));
	
	/* if root or no keylaunchrc, we choose another */
	if (!getuid() || access(keylaunchrc,R_OK))
	{
		g_free(keylaunchrc);
		keylaunchrc = g_strdup ("/etc/keylaunchrc");
	}

	/* read from configfile and set buttons */
	fd = fopen (keylaunchrc, "r");
	if (fd != NULL)
	{
		/* load from configfile */
		while (getline(&buffer, &len, fd) > 0)
		{
			for (i = 0; i < NUM_BUTTONS; i++)
				if (strlen(buffer) && (buffer[0] !='#') && 
					strstr(buffer, buttondefs[i].key) &&
					!strstr(buffer, "???Held") &&
					!strstr(buffer, "???Combine"))
				{
					slash = strrchr (buffer, ':');
					if (slash && ((slash - buffer) > 0))
					{
						slash++;
						if (slash[strlen(slash)-1] == '\n')
							slash[strlen(slash)-1] = 0;
						g_free(buttondefs[i].command);
						buttondefs[i].command = g_strdup(slash);
					}
				}
		}
		fclose (fd);
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
	int i;
	char *bname;
	
	if ((nr == active_button) ||
		((active_button < 0) && (active_button >= NUM_BUTTONS)))
		return;	
	
	g_free(buttondefs[active_button].command);
	buttondefs[active_button].command = 
		g_strdup(gtk_entry_get_text(GTK_ENTRY(self.edit)));
	gtk_entry_set_text(GTK_ENTRY(self.edit), buttondefs[nr].command);
	active_button = nr;
	
	bname = g_strdup_printf(PREFIX "/share/pixmaps/%s", buttondefs[nr].symbol);
	gtk_image_set_from_file(GTK_IMAGE(self.icon), bname);
	g_free(bname);
	
	for (i = 1; i < NUM_COMMANDS; i++)
		if (!strcmp(buttondefs[nr].command, commands[i].command))
		{
			gtk_option_menu_set_history(GTK_OPTION_MENU(self.select), i);
			return;
		}
	gtk_option_menu_set_history(GTK_OPTION_MENU(self.select), 0);
}


void
on_edit_changed (GtkWidget * edit, gpointer user_data)
{
	int i;
	
	if (menuselect)
		return;
	
	for (i = 1; i < NUM_COMMANDS; i++)
		if (!strcmp(buttondefs[active_button].command, commands[i].command))
		{
			gtk_option_menu_set_history(GTK_OPTION_MENU(self.select), i);
			return;
		}
	gtk_option_menu_set_history(GTK_OPTION_MENU(self.select), 0);
}


void 
on_menu_select (GtkOptionMenu *menu,  gpointer user_data)
{
	int pos = gtk_option_menu_get_history(menu);
	
	menuselect = TRUE;	
	if ((pos > 0) && (commands[pos].command))
		gtk_entry_set_text(GTK_ENTRY(self.edit), commands[pos].command);
	else
		gtk_entry_set_text(GTK_ENTRY(self.edit), "");			
	menuselect = FALSE;	
}


GtkWidget *
Keyctl_Build_Objects ()
{
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *layout1 = gtk_layout_new(NULL, NULL);
	GtkWidget *scroll =	gtk_scrolled_window_new(NULL, NULL);
	GtkWidget *bFile = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	GtkWidget *table = gtk_table_new(3, 2, FALSE);
	GtkWidget *cmenu = gtk_menu_new();
	int i;

	self.buttons = NULL;	
	commands_load();
	layout_load();
	
	gtk_menu_set_screen(GTK_MENU(cmenu), NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
	                               GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_table_set_col_spacings(GTK_TABLE(table), 2);
	gtk_container_set_border_width (GTK_CONTAINER(table), 2);
	
	self.edit = gtk_entry_new();
	self.select = gtk_option_menu_new();
	self.icon = gtk_image_new();
	
	for (i = 0; i < NUM_COMMANDS; i++)
		gtk_menu_shell_append(GTK_MENU_SHELL(cmenu),
			gtk_menu_item_new_with_label(commands[i].title));
	
	gtk_option_menu_set_menu(GTK_OPTION_MENU(self.select), cmenu);
	gtk_option_menu_set_history(GTK_OPTION_MENU(self.select), 0);
	
	gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);
	
	gtk_table_attach(GTK_TABLE(table),self.icon, 0, 1, 0, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table),self.edit, 1, 2, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table),self.select, 1, 4, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table),bFile, 3, 4, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	
	if (NUM_BUTTONS)
	{
		self.buttons = g_malloc(NUM_BUTTONS * sizeof(GtkWidget*));
		self.buttons[0] = gtk_radio_button_new(NULL);
		for (i=1; i<NUM_BUTTONS; i++)
			self.buttons[i] = 
				gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(self.buttons[0]));
	}
	gtk_container_add (GTK_CONTAINER (scroll), layout1);
	gtk_container_set_border_width (GTK_CONTAINER (layout1), 1);

	if (bg_image)
	{
		GtkWidget *pixmap1 = gtk_image_new_from_file(bg_image);
		if (pixmap1) 
			gtk_layout_put(GTK_LAYOUT(layout1), pixmap1, bg_x, bg_y);
	}

	for (i = 0; i < NUM_BUTTONS; i++)
	{
		gtk_layout_put (GTK_LAYOUT (layout1), self.buttons[i], buttondefs[i].x, buttondefs[i].y);
		g_signal_connect_after (G_OBJECT (self.buttons[i]), "clicked",
		                        G_CALLBACK(on_button_select),(void *)i);
	}	
	
	g_signal_connect_after (G_OBJECT (bFile), "clicked",
				   G_CALLBACK(on_button_clicked), bFile);
	g_signal_connect(G_OBJECT(self.select), "changed",
	                 G_CALLBACK(on_menu_select), NULL);
	g_signal_connect_after(G_OBJECT(self.edit), "changed",
	                       G_CALLBACK(on_edit_changed), NULL);
	init_buttons ();
	if (self.buttons && self.buttons[0])
	{
		gchar *bname;
		
		gtk_widget_grab_focus(self.buttons[0]);
		gtk_entry_set_text(GTK_ENTRY(self.edit), buttondefs[0].command);
		bname = g_strdup_printf(PREFIX "/share/pixmaps/%s", buttondefs[0].symbol);
		gtk_image_set_from_file(GTK_IMAGE(self.icon), bname);
		g_free(bname);
	}
	return vbox;
}


void Keyctl_Free_Objects ();

/* save new config, force keyctl to reload, and exit */
void
Keyctl_Save ()
{
	int i,j;
	FILE *fd;
	char *cont = NULL;
	GError *err = NULL;
	char **cfglines = NULL;
	gsize len = 0;
	pid_t p_kl;

	/* get current edit value */
	g_free(buttondefs[active_button].command);
	buttondefs[active_button].command = 
		g_strdup(gtk_entry_get_text(GTK_ENTRY(self.edit)));
	
	/* User will write to keylaunch in homedir, root global */	
	if (getuid())
	{
		keylaunchrc =
			g_strdup_printf ("%s/.keylaunchrc", g_get_home_dir());
		if (access(keylaunchrc, F_OK))
		{
			cont = g_strdup_printf("/bin/cp /etc/keylaunchrc %s", keylaunchrc);
			system(cont);
			g_free(cont);
			cont = NULL;
		}
	}
	else
		keylaunchrc = g_strdup ("/etc/keylaunchrc");

	if (g_file_get_contents(keylaunchrc, &cont, &len, &err))
	{
		cfglines = g_strsplit(cont, "\n", 255);
		g_free(cont);
		if (err) 
			g_error_free(err);
	}
	i = 0;
	j = 0;
	
	if (cfglines)
		while (cfglines[j])
		{
			for (i=0; i<NUM_BUTTONS; i++)
			{
				if (cfglines[j] && strlen(cfglines[j]) && (cfglines[j][0] !='#') && 
					strstr(cfglines[j], buttondefs[i].key) &&
					!strstr(cfglines[j], "???Held") &&
					!strstr(cfglines[j], "???Combine"))
				{
					g_free(cfglines[j]);
					if ((buttondefs[i].command && strlen(buttondefs[i].command)))
						cfglines[j] = 
							g_strdup_printf("key=%s %s:-:%s", buttondefs[i].modificator,
											buttondefs[i].key, buttondefs[i].command);
					else
						cfglines[j] = NULL;
					buttondefs[i].type = 0xff;
				}
			}
			j++;	
		}
	
	for (i=0; i<NUM_BUTTONS; i++)
	{
		if (buttondefs[i].type != 0xff)
		{
			j++;
			cfglines = realloc(cfglines, (j+1) * sizeof(char*));
			cfglines[j-1] =
				g_strdup_printf("key=%s %s:-:%s", buttondefs[i].modificator,
				                buttondefs[i].key, buttondefs[i].command);
			cfglines[j] = NULL;
		}
	}
	fd = fopen (keylaunchrc, "w");
	if (fd == NULL)
	{
		gpe_error_box (_("Critical: Can't open keylaunchrc for writing!"));
		return;
	}
	
	for (i=0; i<j; i++)
	{
		if (strlen(cfglines[i]) && (cfglines[i][strlen(cfglines[i])-1] == '\n'))
			fprintf(fd, "%s", cfglines[i]);
		else
			fprintf(fd, "%s\n", cfglines[i]);
	}
	fclose (fd);
	if (cfglines)
		g_strfreev(cfglines);
	
	/* try to get rid of running keylaunch */
	system("/usr/bin/killall keylaunch");
	
	/* start a new keylaunch process */
	p_kl = fork();
	switch (p_kl)
	{
		case -1: 
			gpe_error_box (_("Could not restart key handler!"));
		break;
		case  0: 
			execlp(KEYLAUNCH_BIN, NULL);
			exit(0);
		break;
		default: 
		break;
	} 
}

void
Keyctl_Restore ()
{
}


void
FileSelected (char *file, gpointer data)
{
	if (!strlen(file))
		return;
	if (access(file, X_OK))
		gpe_error_box(_("The file you selected is not executable."));
	else
		gtk_entry_set_text(GTK_ENTRY(self.edit), file);
}
