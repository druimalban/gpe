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
 * GPE Administaration and global settings module.
 *
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <libintl.h>
#define _(x) gettext(x)

#include <gtk/gtk.h>

#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>

#include "gpe-admin.h"
#include "applets.h"
#include "suid.h"
#include "cfgfile.h"

/* --- local types and constants --- */


/* --- module global variables --- */

static GtkWidget *cbUserAccess;

/* --- local intelligence --- */

void do_change_user_access(char *acstr)
{
	change_cfg_value (GPE_CONF_CFGFILE, "user_access", acstr, '=');
}


/* --- gpe-conf interface --- */

void GpeAdmin_Free_Objects()
{
}


void GpeAdmin_Save()
{
	gint access = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cbUserAccess));
	gchar* acstr;
	
	acstr = g_strdup_printf("%i",access);
	suid_exec("GAUA",acstr);
	g_free(acstr);
}

void GpeAdmin_Restore()
{
	gchar* acstr;
	acstr = get_file_var(GPE_CONF_CFGFILE,"user_access");
	if (acstr == NULL)
		acstr = "0";
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbUserAccess),atoi(acstr));		
}

GtkWidget*
GpeAdmin_Build_Objects (void)
{
  GtkWidget *vbox1;
  GtkWidget *label1;
  gchar *fstr;
	
  vbox1 = gtk_vbox_new (FALSE, gpe_get_catspacing());
  gtk_container_set_border_width (GTK_CONTAINER (vbox1), gpe_get_border());

  label1 = gtk_label_new (NULL); 
  fstr = g_strdup_printf("%s %s %s","<b>",_("General"),"</b>");
  gtk_label_set_markup (GTK_LABEL(label1),fstr); 
  g_free(fstr);
  gtk_box_pack_start (GTK_BOX (vbox1), label1, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);
    
  cbUserAccess = gtk_check_button_new_with_label (_("Allow users to configure everything"));
  gtk_box_pack_start (GTK_BOX (vbox1), cbUserAccess, FALSE, FALSE, 0);

  GpeAdmin_Restore();
	
  return vbox1;
}
