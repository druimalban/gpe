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

#include "../gpe-appmgr/package.h"
#include "../gpe-appmgr/cfg.h"
#include "applets.h"

#include "appmgr_setup.h"
#include "gpe/errorbox.h"

struct package *config=NULL;

GtkWidget *all_group_check=NULL;
GtkWidget *tab_view_icon=NULL;
GtkWidget *tab_view_list=NULL;
GtkWidget *auto_hide_group_labels_check=NULL;
GtkWidget *show_recent_check=NULL;
GtkWidget *dont_launch_check=NULL;

char dont_launch_file[255];

int dont_launch_exists()
{
  char *home = getenv("HOME");
  if(strlen(home) > 230)
      gpe_error_box( "bad $HOME !!");

  sprintf(dont_launch_file,"%s/.gpe/dont_launch_appmgr",home);
  
  return file_exists(dont_launch_file);
}

void page_add (GtkVBox *vb)
{
	GtkWidget *frame, *lvbox;
	GtkWidget *label;

	cfg_load();

	/* Show the 'All' group */
	all_group_check =
		gtk_check_button_new_with_label ("Show 'All' group");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(all_group_check),
				      cfg_options.show_all_group);

	gtk_box_pack_start_defaults (GTK_BOX(vb), all_group_check);

	/* Autohide group titles */
	auto_hide_group_labels_check =
		gtk_check_button_new_with_label ("Auto-hide group labels");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(auto_hide_group_labels_check),
				      cfg_options.auto_hide_group_labels);
	gtk_box_pack_start_defaults (GTK_BOX(vb), auto_hide_group_labels_check);

	/* Show 'recent' tab */
	show_recent_check =
		gtk_check_button_new_with_label ("Show 'Recent' tab");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(show_recent_check),
				      cfg_options.show_recent_apps);
	gtk_box_pack_start_defaults (GTK_BOX(vb), show_recent_check);

	/* Dont lauch at startup */
	dont_launch_check =
		gtk_check_button_new_with_label ("Dont launch at startup");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON( dont_launch_check),
				      dont_launch_exists());
	gtk_box_pack_start_defaults (GTK_BOX(vb), dont_launch_check);

	/* Tab view type */
	frame = gtk_frame_new ("Tab view");
	gtk_box_pack_start_defaults (GTK_BOX(vb), frame);
	lvbox = gtk_vbox_new (0,0);
	gtk_container_add (GTK_CONTAINER(frame), lvbox);


	/* ->Icon */
	tab_view_icon = gtk_radio_button_new_with_label (NULL, "Icon");
	gtk_box_pack_start_defaults (GTK_BOX(lvbox), tab_view_icon);

	/* ->List */
	tab_view_list = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON(tab_view_icon), "List");
	gtk_box_pack_start_defaults (GTK_BOX(lvbox), tab_view_list);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(tab_view_list),
				      cfg_options.tab_view == TAB_VIEW_LIST);

	/*  Label */
	label = gtk_label_new("Changes will take effect\n after relog");
	gtk_box_pack_start_defaults (GTK_BOX(vb), label);

	gtk_widget_show_all (GTK_WIDGET(vb));
}

void Appmgr_Restore (void){}
void Appmgr_Free_Objects (void){}
void Appmgr_Save (void)
{
	char *fn;

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

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(show_recent_check)))
		package_set_data (config, "show_recent_apps", "yes");
	else
		package_set_data (config, "show_recent_apps", "no");


	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(dont_launch_check)))
	  system_printf("touch %s",dont_launch_file);
	else if(file_exists(dont_launch_file))
	  system_printf("rm %s",dont_launch_file);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(tab_view_icon)))
		package_set_data (config, "tab_view", "icon");
	else
		package_set_data (config, "tab_view", "list");

 	fn = g_strdup_printf ("%s/.gpe/gpe-appmgr", g_get_home_dir());
	package_save (config, fn);
	g_free (fn);

}

GtkWidget *Appmgr_Build_Objects()
{
	GtkWidget  *vbox;

	vbox = gtk_vbox_new (0,0);

	page_add (GTK_VBOX(vbox));

	return vbox;
}
