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
#include <gpe/errorbox.h>
#include <gpe/spacing.h>

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
	GtkWidget *catvbox1;
	GtkWidget *catlabel1;
	GtkWidget *catconthbox1;
	GtkWidget *catindentlabel1;
	GtkWidget *controlvbox1;
	GtkWidget *catvbox2;
	GtkWidget *catlabel2;
	GtkWidget *catconthbox2;
	GtkWidget *catindentlabel2;
	GtkWidget *controlvbox2;
	GSList *controlvbox2_group = NULL;
	
	GtkWidget *label;

	gchar *gpe_catindent = gpe_get_catindent ();
	guint gpe_boxspacing = gpe_get_boxspacing ();

	cfg_load();

	/* -------------------------------------------------------------------------- */
	catvbox1 = gtk_vbox_new (FALSE, gpe_boxspacing);
	gtk_box_pack_start (GTK_BOX (vb), catvbox1, TRUE, TRUE, 0);
	
	catlabel1 = gtk_label_new (_("General")); /* FIXME: GTK2: make this bold */
	gtk_box_pack_start (GTK_BOX (catvbox1), catlabel1, FALSE, FALSE, 0);
	gtk_label_set_justify (GTK_LABEL (catlabel1), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (catlabel1), 0, 0.5);
	
	catconthbox1 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (catvbox1), catconthbox1, TRUE, TRUE, 0);
	
	catindentlabel1 = gtk_label_new (gpe_catindent);
	gtk_box_pack_start (GTK_BOX (catconthbox1), catindentlabel1, FALSE, FALSE, 0);
	
	controlvbox1 = gtk_vbox_new (FALSE, gpe_boxspacing);
	gtk_box_pack_start (GTK_BOX (catconthbox1), controlvbox1, TRUE, TRUE, 0);
	
	
	/* Show the 'All' group */
	all_group_check =
		gtk_check_button_new_with_label ("Show 'All' group");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(all_group_check),
				      cfg_options.show_all_group);

	gtk_box_pack_start (GTK_BOX(controlvbox1), all_group_check, FALSE, FALSE, 0);

	/* Autohide group titles */
	auto_hide_group_labels_check =
		gtk_check_button_new_with_label ("Auto-hide group labels");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(auto_hide_group_labels_check),
				      cfg_options.auto_hide_group_labels);
	gtk_box_pack_start (GTK_BOX(controlvbox1), auto_hide_group_labels_check, FALSE, FALSE, 0);

	/* Show 'recent' tab */
	show_recent_check =
		gtk_check_button_new_with_label ("Show 'Recent' tab");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(show_recent_check),
				      cfg_options.show_recent_apps);
	gtk_box_pack_start (GTK_BOX(controlvbox1), show_recent_check, FALSE, FALSE, 0);

	/* Don't launch at startup */
	dont_launch_check =
		gtk_check_button_new_with_label ("Do not launch at startup");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON( dont_launch_check),
				      dont_launch_exists());
	gtk_box_pack_start (GTK_BOX(controlvbox1), dont_launch_check, FALSE, FALSE, 0);

	/* -------------------------------------------------------------------------- */
	catvbox2 = gtk_vbox_new (FALSE, gpe_boxspacing);
	gtk_box_pack_start (GTK_BOX (vb), catvbox2, TRUE, TRUE, 0);
	
	catlabel2 = gtk_label_new (_("Tab View")); /* FIXME: GTK2: make this bold */
	gtk_box_pack_start (GTK_BOX (catvbox2), catlabel2, FALSE, FALSE, 0);
	gtk_label_set_justify (GTK_LABEL (catlabel2), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (catlabel2), 0, 0.5);
	
	catconthbox2 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (catvbox2), catconthbox2, TRUE, TRUE, 0);
	
	catindentlabel2 = gtk_label_new (gpe_catindent);
	gtk_box_pack_start (GTK_BOX (catconthbox2), catindentlabel2, FALSE, FALSE, 0);
	
	controlvbox2 = gtk_vbox_new (FALSE, gpe_boxspacing);
	gtk_box_pack_start (GTK_BOX (catconthbox2), controlvbox2, TRUE, TRUE, 0);


	tab_view_icon = gtk_radio_button_new_with_label (controlvbox2_group, _("Icon"));
	controlvbox2_group = gtk_radio_button_group (GTK_RADIO_BUTTON (tab_view_icon));
	gtk_box_pack_start (GTK_BOX (controlvbox2), tab_view_icon, FALSE, FALSE, 0);
	
	tab_view_list = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON(tab_view_icon), _("List"));
	controlvbox2_group = gtk_radio_button_group (GTK_RADIO_BUTTON (tab_view_list));
	gtk_box_pack_start (GTK_BOX (controlvbox2), tab_view_list, FALSE, FALSE, 0);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(tab_view_list),
				      cfg_options.tab_view == TAB_VIEW_LIST);
	
	/* -------------------------------------------------------------------------- */
	/*  Label */
	label = gtk_label_new(_("Changes will take effect after relog"));
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
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

	system("kill -HUP  `pidof gpe-appmgr`");

}

GtkWidget *Appmgr_Build_Objects()
{
	GtkWidget  *vbox;

        guint gpe_catspacing = gpe_get_catspacing ();
	guint gpe_border = gpe_get_border ();
  
	vbox = gtk_vbox_new (FALSE, gpe_catspacing);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), gpe_border);

	page_add (GTK_VBOX(vbox));

	return vbox;
}
