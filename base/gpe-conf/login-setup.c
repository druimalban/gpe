/*
 * login-setup module for gpe-conf
 *
 * Copyright (C) 2002 Colin Marquardt <ipaq@marquardt-home.de>
 *               2004 Florian Boor <florian@kernelconcepts.de>
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
#include <gtk/gtk.h>
#include <time.h>
#include <unistd.h> /* for readlink () */

#include <gpe/errorbox.h>
#include <gpe/pixmaps.h>
#include <gpe/spacing.h>

#include "applets.h"
#include "misc.h"
#include "login-setup.h"
#include "suid.h"

#define FILE_GPELOGIN "/etc/sysconfig/gpelogin"
 
GtkWidget *login_bg_show_check  = NULL;
GtkWidget *ownerinfo_show_check = NULL;
GtkWidget *autologin_check = NULL;
GtkWidget *login_bg_pixmap;
GtkWidget *login_lock_display_check;
GtkWidget *controlvbox1;

gboolean login_lock_display = FALSE;
gboolean ownerinfo_show = FALSE;
gboolean autologin = FALSE;

gboolean ownerinfo_show_initial = FALSE;
gboolean login_lock_display_initial  = FALSE;
gboolean autologin_initial  = FALSE;

gboolean ownerinfo_show_writable = FALSE;
gboolean login_lock_script_writable  = FALSE;

guint buttonwidth, buttonheight = 42;
guint hsync = 36;

static gchar login_bg_filename[PATH_MAX + 1] = "<none>";

void update_login_lock (GtkWidget *togglebutton, gpointer user_data);
void update_autologin (GtkWidget *togglebutton, gpointer user_data);

static gboolean
get_autologin_setting(void)
{
	gchar val[128];
	FILE *fp;
	gboolean result = FALSE;

	memset(val, 0, 128);
		
	fp = fopen(FILE_GPELOGIN, "r");
	
	if (fp)
	{
		while (fgets(val, 128, fp))
		{
			gchar *vp;
			if ((vp = strstr(val, "AUTOLOGIN=\"")))
			{	
				if (!strncasecmp(vp + 11, "true", 4))
					result = TRUE;
				break;
			}
		}
		fclose(fp);
	}
	return result;
}

void
set_autologin_setting(gboolean state)
{
	change_cfg_value(FILE_GPELOGIN, 
	                 "AUTOLOGIN", 
	                 state ? "\"true\"" : "\"false\"", '=');
}

static
gboolean have_password(void)
{
	struct passwd *pw;
	
	pw = getpwuid(getuid());
	
	if (pw)
		if ((pw->pw_passwd) && (strlen(pw->pw_passwd)))
			return TRUE;
	
	return FALSE;
}


GtkWidget *Login_Setup_Build_Objects()
{
  GtkWidget *mainvbox;
  GtkWidget *rootwarn_hbox;
  GtkWidget *rootwarn_icon;
  GtkWidget *rootwarn_label;
  
  GtkWidget *categories;
  GtkWidget *catvbox1;
  GtkWidget *catlabel1;
  GtkWidget *catconthbox1;
  GtkWidget *catindentlabel1;
  
  GdkPixbuf *pixbuf;

  gchar *gpe_catindent = gpe_get_catindent ();
  guint gpe_catspacing = gpe_get_catspacing ();
  guint gpe_boxspacing = gpe_get_boxspacing ();
  guint gpe_border     = gpe_get_border ();

  gchar *tstr;

  if (suid_exec("CHEK"," "))
  {
    ownerinfo_show_writable = FALSE;
    login_lock_script_writable = FALSE;
  }
  else
  {
    ownerinfo_show_writable = TRUE;
    login_lock_script_writable = TRUE;
  }
  /* ======================================================================== */
  /* draw the GUI */

  /* the vbox which can hold the warning hbox (containing icon and text) */
  mainvbox = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_widget_show (mainvbox);
  gtk_container_set_border_width (GTK_CONTAINER (mainvbox), gpe_border);
  
  rootwarn_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (rootwarn_hbox);

  rootwarn_label = gtk_label_new (_("Some of these settings can only be changed by the user 'root'."));
  gtk_widget_show (rootwarn_label);
  gtk_label_set_justify (GTK_LABEL (rootwarn_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (rootwarn_label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (rootwarn_label), 0, 0);

  pixbuf = gpe_find_icon ("warning16");
  rootwarn_icon = gtk_image_new_from_pixbuf (pixbuf);
  gtk_misc_set_alignment (GTK_MISC (rootwarn_icon), 0, 0);
  gtk_box_pack_start (GTK_BOX (rootwarn_hbox), rootwarn_icon, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (rootwarn_hbox), rootwarn_label, TRUE, TRUE, gpe_boxspacing);
  if (!ownerinfo_show_writable)
    gtk_box_pack_start (GTK_BOX (mainvbox), rootwarn_hbox, FALSE, TRUE, 0);
   
  /* -------------------------------------------------------------------------- */
  categories = gtk_vbox_new (FALSE, gpe_catspacing);
  gtk_box_pack_start (GTK_BOX (mainvbox), categories, TRUE, TRUE, 0);
  
  catvbox1 = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (categories), catvbox1, TRUE, TRUE, 0);
  
  catlabel1 = gtk_label_new (NULL);
  
  tstr = g_strdup_printf("<b>%s</b>",_("General"));
  gtk_label_set_markup(GTK_LABEL(catlabel1),tstr);
  g_free(tstr);
  gtk_box_pack_start (GTK_BOX (catvbox1), catlabel1, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (catlabel1), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (catlabel1), 0, 0.5);
  
  catconthbox1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (catvbox1), catconthbox1, TRUE, TRUE, 0);
  
  catindentlabel1 = gtk_label_new (gpe_catindent);
  gtk_box_pack_start (GTK_BOX (catconthbox1), catindentlabel1, FALSE, FALSE, 0);
  
  controlvbox1 = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (catconthbox1), controlvbox1, TRUE, TRUE, 0);
  
  ownerinfo_show_check =
    gtk_check_button_new_with_label (_("Show owner information at login."));
	
  if (have_password())
    login_lock_display_check =
      gtk_check_button_new_with_label (_("Lock display on suspend."));
  else
  {
    login_lock_display_check =
      gtk_check_button_new_with_label (_("Lock display on suspend. \n  (needs password)"));
	gtk_widget_set_sensitive(GTK_WIDGET(login_lock_display_check), FALSE);
  }

  autologin_check =
    gtk_check_button_new_with_label (_("Automatic login. \n  (without password)"));
  
  /* check the dontshow files to set initial values for the checkboxes */
  get_initial_values();
  ownerinfo_show = ownerinfo_show_initial;
  login_lock_display  = login_lock_display_initial;
  autologin  = autologin_initial;

  gtk_box_pack_start (GTK_BOX(controlvbox1), ownerinfo_show_check, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(controlvbox1), login_lock_display_check, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(controlvbox1), autologin_check, FALSE, FALSE, 0);

  /* ------------------------------------------------------------------------ */
  g_signal_connect (G_OBJECT (ownerinfo_show_check), "toggled",
                    G_CALLBACK (update_ownerinfo_show), NULL);
  
  g_signal_connect (G_OBJECT (login_lock_display_check), "toggled",
                    G_CALLBACK (update_login_lock), NULL);

  g_signal_connect (G_OBJECT (autologin_check), "toggled",
                    G_CALLBACK (update_autologin), NULL);
					  
  if (ownerinfo_show_writable)
  {
    gtk_widget_set_sensitive (ownerinfo_show_check, TRUE);
    gtk_widget_set_sensitive (autologin_check, TRUE);
  }
  else 
  {
    gtk_widget_set_sensitive (ownerinfo_show_check, FALSE);
    gtk_widget_set_sensitive (autologin_check, TRUE);
  }
  return mainvbox;
}

void
Login_Setup_Free_Objects ()
{
}

void
Login_Setup_Save ()
{
  char tmp[255];
  /* other settings apply immediately without saving, no explicit save necessary */
  /* here we should check if save is necessary */
  if (!access(login_bg_filename, F_OK))
  {
  	snprintf(tmp, 255, "%s", login_bg_filename);
  	suid_exec("ULBF", tmp);
  }
}

void
Login_Setup_Restore ()
{
  g_message ("Requested explicit restoration of initial values.");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(ownerinfo_show_check),
				ownerinfo_show_initial);
  ownerinfo_show = ownerinfo_show_initial;
	
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(login_lock_display_check),
				login_lock_display_initial);
  login_lock_display = login_lock_display_initial;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(autologin_check),
				autologin_initial);
  autologin = autologin_initial;
}


void
get_initial_values ()
{
  g_message ("Checking the dontshow files to set initial values for the checkboxes.");

  /* check if the dontshow files are there */
  if (access (GPE_OWNERINFO_DONTSHOW_FILE, F_OK) == 0)
    ownerinfo_show_initial = FALSE;
  else
    ownerinfo_show_initial = TRUE;

  if (access(GPE_LOGIN_LOCK_SCRIPT, X_OK) < 0) 	
    login_lock_display_initial = FALSE;
  else
    login_lock_display_initial = TRUE;
  
  autologin_initial = get_autologin_setting();
  
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(ownerinfo_show_check),
				ownerinfo_show_initial);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(login_lock_display_check),
				login_lock_display_initial);
  
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(autologin_check),
				autologin_initial);
}

void 
update_ownerinfo_show ()
{
  char uc[3];
	
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(ownerinfo_show_check)))
    ownerinfo_show = TRUE;
  else
    ownerinfo_show = FALSE;

  sprintf(uc, "%i", ownerinfo_show);
  suid_exec("UOIS", uc);
}


void 
update_login_lock (GtkWidget *togglebutton, gpointer  user_data)
{
  char uc[3];
  login_lock_display = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(togglebutton));

  sprintf(uc, "%i", login_lock_display);
  suid_exec("ULDS", uc);
}

void 
update_autologin (GtkWidget *togglebutton, gpointer  user_data)
{
  char uc[3];
	
  autologin = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(togglebutton));
  sprintf(uc, "%i", autologin);
  suid_exec("SALI", uc);
}
