/* gpe-appmgr - a program launcher

   Copyright 2002 Robert Mibus;

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* for stat() */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "package.h"

struct package *config=NULL;

GtkWidget *all_group_check=NULL;
GtkWidget *tab_view_icon=NULL;
GtkWidget *tab_view_list=NULL;
GtkWidget *auto_hide_group_labels_check=NULL;

void page_add (GtkVBox *vb)
{
	GtkWidget *frame, *lvbox;
	char *fn;
	char *s;

	fn = g_strdup_printf ("%s/.gpe/gpe-appmgr", g_get_home_dir());
	if (fn)
		config = package_read (fn);
	if (!config)
		config = package_read ("/usr/share/gpe/config/gpe-appmgr");
	g_free (fn);

	/* Show the 'All' group */
	all_group_check =
		gtk_check_button_new_with_label ("Show 'All' group");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(all_group_check),
				      FALSE);

	if ((s = package_get_data (config, "show_all_group")))
		switch (tolower(*s))
		{
		case '1':
		case 'y':
		case 't':
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(all_group_check),
						      TRUE);
		}
	gtk_box_pack_start_defaults (GTK_BOX(vb), all_group_check);

	/* Autohide group titles */
	auto_hide_group_labels_check =
		gtk_check_button_new_with_label ("Auto-hide group labels");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(auto_hide_group_labels_check),
				      FALSE);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(auto_hide_group_labels_check),
				      TRUE);
	if ((s = package_get_data (config, "auto_hide_group_labels")))
		switch (tolower(*s))
		{
		case '1':
		case 'y':
		case 't':
			break;
		default:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(auto_hide_group_labels_check),
						      FALSE);
		}
	gtk_box_pack_start_defaults (GTK_BOX(vb), auto_hide_group_labels_check);


	/* Tab view type */
	frame = gtk_frame_new ("Tab view");
	gtk_box_pack_start_defaults (GTK_BOX(vb), frame);
	lvbox = gtk_vbox_new (0,0);
	gtk_container_add (GTK_CONTAINER(frame), lvbox);
	tab_view_icon = gtk_radio_button_new_with_label (NULL, "Icon");
	gtk_box_pack_start_defaults (GTK_BOX(lvbox), tab_view_icon);
	tab_view_list = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON(tab_view_icon), "List");
	gtk_box_pack_start_defaults (GTK_BOX(lvbox), tab_view_list);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(tab_view_list),
				      FALSE);

	if ((s = package_get_data (config, "tab_view")))
        {
                switch (tolower(*s))
                {
                case 'l':
                        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(tab_view_list),
						      TRUE);
                }
        }

	gtk_widget_show_all (GTK_WIDGET(vb));
}

void page_close (void)
{
	char *fn;
	struct stat buf;

	if (!config)
	{
		config = (struct package *) package_new ();
		config->name = (char*) strdup ("package");
		config->data = (char*) strdup("gpe-appmgr");
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(all_group_check)))
		package_set_data (config, "show_all_group", "yes");
	else
		package_set_data (config, "show_all_group", "no");

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(auto_hide_group_labels_check)))
		package_set_data (config, "auto_hide_group_labels", "yes");
	else
		package_set_data (config, "auto_hide_group_labels", "no");

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(tab_view_icon)))
		package_set_data (config, "tab_view", "icon");
	else
		package_set_data (config, "tab_view", "list");

	fn = g_strdup_printf ("%s/.gpe", g_get_home_dir());
	if (stat (fn, &buf) != 0)
	{
		if (mkdir (fn, 0) != 0)
		{
			perror ("Cannot create ~/.gpe");
			exit (1);
		}
	} else {
		if (!S_ISDIR(buf.st_mode))
		{
			printf ("ERROR: ~/.gpe is not a directory!\n");
			exit (1);
		}
	}

	g_free (fn);
 	fn = g_strdup_printf ("%s/.gpe/gpe-appmgr", g_get_home_dir());
	package_save (config, fn);
	g_free (fn);

	system ("kill -HUP `ps -C gpe-appmgr --no-heading -o %p`");
}

int on_close ()
{
	page_close ();
	gtk_main_quit ();
	return TRUE;
}

int main (int argc, char **argv)
{
	GtkWidget *win, *vbox;

	gtk_init (&argc, &argv);

	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect (GTK_OBJECT(win),
			    "destroy",
			    (GtkSignalFunc)on_close,
			    NULL);

	vbox = gtk_vbox_new (0,0);
	gtk_container_add (GTK_CONTAINER(win), vbox);

	gtk_widget_show_all (win);

	page_add (GTK_VBOX(vbox));

	gtk_main ();

	return 0;
}
