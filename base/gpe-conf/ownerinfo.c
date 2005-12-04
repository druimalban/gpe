/*
 * ownerinfo module for gpe-conf
 *
 * Copyright (C) 2002 Colin Marquardt <ipaq@marquardt-home.de>
 *         2004, 2005 Florian Boor <florian@kernelconcepts.de>
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

#include <gpe/errorbox.h>
#include <gpe/pixmaps.h>
#include <gpe/spacing.h>

#include "applets.h"
#include "misc.h"
#include "ownerinfo.h"

static
void File_Selected (char *file,
		    gpointer data);

#define CURRENT_DATAFILE_VER 2

#define INFO_MATCH           "[gpe-ownerinfo data version "

#define UPGRADE_ERROR      -1
#define UPGRADE_NOT_NEEDED  0
#define GPE_LOGIN_STARTUP	"/etc/X11/Xsession.d/50autolock"

#ifndef IMG_HEIGHT
  #define IMG_HEIGHT 64
  #define IMG_WIDTH  64
#endif

GtkWidget *photofile;
GtkWidget *name;
GtkWidget *email;
GtkWidget *phone;
GtkWidget *address;
GtkWidget *photo;
GtkWidget *button;

static gchar *ownerphotofile, *ownername, *owneremail, *ownerphone, *owneraddress;

FILE *fp;


static void
restart_gpe_login(void)
{
	system("killall gpe-login");
	system(GPE_LOGIN_STARTUP);
}

GtkWidget *Ownerinfo_Build_Objects()
{
  GtkWidget *table, *lHeader;
  GtkWidget *vbox, *hbox, *label, *icon;
  GtkWidget *scrolledwindow1;
  GtkWidget *viewport1;
  GtkWidget *owner_photofile_label;
  GtkWidget *owner_name_label;
  GtkWidget *owner_email_label;
  GtkWidget *owner_phone_label;
  GtkWidget *owner_address_label;
  GdkPixbuf *pixbuf;
  gint upgrade_result = UPGRADE_ERROR;
  GtkAttachOptions table_attach_left_col_x;
  GtkAttachOptions table_attach_left_col_y;
  GtkAttachOptions table_attach_right_col_x;
  GtkAttachOptions table_attach_right_col_y;
  GtkJustification table_justify_left_col;
  GtkJustification table_justify_right_col;
  guint gpe_border     = gpe_get_border ();
  guint gpe_boxspacing = gpe_get_boxspacing ();
  gboolean datafile_writable = FALSE;
  gchar *tstr = NULL;
  
  /* ======================================================================== */
  /* get the data from the file if existant */
  ownerphotofile = g_strdup ("ownerphoto");
  ownername      = g_strdup ("GPE User");
  owneremail     = g_strdup ("nobody@localhost.localdomain");
  ownerphone     = g_strdup ("+99 (9999) 999-9999");
  owneraddress   = g_strdup ("Use Settings->Owner Information to " \
  			"change.\nData stored in\n/etc/gpe/gpe-ownerinfo.data.");

  /* 
   * GTK_EXPAND  the widget should expand to take up any extra space
                 in its container that has been allocated.
   * GTK_SHRINK  the widget should shrink as and when possible.
   * GTK_FILL    the widget should fill the space allocated to it.
   */
  
  table_attach_left_col_x = GTK_SHRINK | GTK_FILL; 
  table_attach_left_col_y = 0;
  table_attach_right_col_x = GTK_EXPAND | GTK_FILL;
  table_attach_right_col_y = GTK_FILL;
  
  table_justify_left_col = GTK_JUSTIFY_LEFT;
  table_justify_right_col = GTK_JUSTIFY_LEFT;

  fp = fopen (GPE_OWNERINFO_DATA, "r");
  if (fp)
    {
      char buf[2048];
      guint numchar;
      gchar *firstline;

      upgrade_result = maybe_upgrade_datafile ();

      if (upgrade_result == UPGRADE_NOT_NEEDED) {	
	/*  we have at least version 2, so we need to skip the 1st line
	 *  and read the photofile file name:
	 */
	if (fgets (buf, sizeof (buf), fp))
	  {
	    firstline = g_strdup (buf);
	    firstline[strlen (firstline)-1]='\0';
	  }
	if (fgets (buf, sizeof (buf), fp))
	  {
	    ownerphotofile = g_strdup (buf);
	    ownerphotofile[strlen (ownerphotofile)-1]='\0';
	    if (access (ownerphotofile, R_OK) == 0)
	      my_icons[9].filename = g_strdup (ownerphotofile);
	    else
	      fprintf (stderr, "gpe-conf ownerinfo: file '%s' could not be found, using default.\n",
		       ownerphotofile);
	  }
      }
      if (fgets (buf, sizeof (buf), fp))
        {
          ownername = g_strdup (buf);
          ownername[strlen (ownername)-1]='\0';
        }
      if (fgets (buf, sizeof (buf), fp))
        {
          owneremail = g_strdup (buf);
          owneremail[strlen (owneremail)-1]='\0';
        }
      if (fgets (buf, sizeof (buf), fp))
        {
	  ownerphone = g_strdup (buf);
	  ownerphone[strlen (ownerphone)-1]='\0';
        }
      /* the rest is taken as address: */
      if ((numchar=fread (buf, 1, sizeof (buf), fp)))
      	{
	  owneraddress = g_malloc (numchar + 1);
	  memcpy (owneraddress, buf, numchar);
	  owneraddress[numchar] = '\0';
      	}

      fclose (fp);
    }
  else /* fp == NULL, no file existing */
    {
      /* it's just okay if the file doesn't exist */
    }

  /*
   * Check if we are even allowed to write to the owner data file
   *    or create it if necessary.
   * FIXME:
   *    Gotta do the same at top level of gpe-conf since the buttons
   *    are not insensitive otherwise.
   */

  /* either we can write to the file... */
  
  if ((access (GPE_OWNERINFO_DATA, W_OK) == 0) ||
      (((access (gpe_dirname (g_strdup(GPE_OWNERINFO_DATA)), W_OK)) == 0) &&
       (access (GPE_OWNERINFO_DATA, F_OK) != 0)) || !suid_exec("CHEK",""))
    datafile_writable = TRUE;
  else
    datafile_writable = FALSE;

  /* ======================================================================== */
  /* draw the GUI */

  /*
   * Use 2*gpe_boxspacing here because this is a single width,
   * whereas normally gpe_boxspacing is around *each* of two adjacent widgets:
   */
  vbox = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), gpe_border);
  
  hbox = gtk_hbox_new (FALSE, 0);

  lHeader = gtk_label_new (NULL);
  tstr = g_strdup_printf("<b>%s</b>",_("Information about the Owner"));
  gtk_label_set_markup(GTK_LABEL(lHeader),tstr);
  g_free(tstr);
  
  gtk_label_set_justify (GTK_LABEL (lHeader), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (lHeader), 0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), lHeader, FALSE, TRUE, 0);
  
  label = gtk_label_new (_("Only the user 'root' is allowed to change this information"));
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  pixbuf = gpe_find_icon ("warning16");
  icon = gtk_image_new_from_pixbuf (pixbuf);
  gtk_misc_set_alignment (GTK_MISC (icon), 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 2*gpe_boxspacing);
  if (!datafile_writable)
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
 
  table = gtk_table_new (6, 2, FALSE);
  gtk_widget_set_name (table, "table");
  gtk_table_set_col_spacings (GTK_TABLE (table), 2*gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

  /* ------------------------------------------------------------------------ */
  owner_name_label = gtk_label_new (_("Name:"));
  gtk_table_attach (GTK_TABLE (table), owner_name_label, 0, 1, 0, 1,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  gtk_label_set_justify (GTK_LABEL (owner_name_label), table_justify_left_col);
  gtk_misc_set_alignment (GTK_MISC (owner_name_label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (owner_name_label), 0, gpe_boxspacing);

  name = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (name), ownername);
  gtk_table_attach (GTK_TABLE (table), name, 1, 2, 0, 1,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, 0);

  /* ------------------------------------------------------------------------ */
  owner_email_label = gtk_label_new (_("E-Mail:"));
  gtk_table_attach (GTK_TABLE (table), owner_email_label, 0, 1, 1, 2,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  gtk_label_set_justify (GTK_LABEL (owner_email_label), table_justify_left_col);
  gtk_misc_set_alignment (GTK_MISC (owner_email_label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (owner_email_label), 0, gpe_boxspacing);

  email = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (email), owneremail);
  gtk_table_attach (GTK_TABLE (table), email, 1, 2, 1, 2,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, 0);

  /* ------------------------------------------------------------------------ */
  owner_phone_label = gtk_label_new (_("Phone:"));
  gtk_table_attach (GTK_TABLE (table), owner_phone_label, 0, 1, 2, 3,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  gtk_label_set_justify (GTK_LABEL (owner_phone_label), table_justify_left_col);
  gtk_misc_set_alignment (GTK_MISC (owner_phone_label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (owner_phone_label), 0, gpe_boxspacing);

  phone = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (phone), ownerphone);
  gtk_table_attach (GTK_TABLE (table), phone, 1, 2, 2, 3,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, 0);

  /* ------------------------------------------------------------------------ */
  owner_address_label = gtk_label_new (_("Address:"));
  gtk_table_attach (GTK_TABLE (table), owner_address_label, 0, 1, 3, 4,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y | GTK_FILL ),
		    0, gpe_boxspacing);
  gtk_label_set_justify (GTK_LABEL (owner_address_label), table_justify_left_col);
  gtk_misc_set_alignment (GTK_MISC (owner_address_label), 0, 0);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_table_attach (GTK_TABLE (table), scrolledwindow1, 1, 2, 3, 4,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_x),
		    0, gpe_boxspacing);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  viewport1 = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), viewport1);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport1), GTK_SHADOW_NONE);

  
  address = gtk_text_view_new (); 
  gtk_container_add (GTK_CONTAINER (viewport1), address);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (address), TRUE);
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(address)),owneraddress,-1);
  /* ------------------------------------------------------------------------ */
  owner_photofile_label = gtk_label_new (_("Photofile:"));
  gtk_table_attach (GTK_TABLE (table), owner_photofile_label, 0, 1, 4, 5,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  gtk_label_set_justify (GTK_LABEL (owner_photofile_label), table_justify_left_col);
  gtk_misc_set_alignment (GTK_MISC (owner_photofile_label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (owner_photofile_label), 0, gpe_boxspacing);

  photofile = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (photofile), ownerphotofile);

  gtk_table_attach (GTK_TABLE (table), photofile, 1, 2, 4, 5,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, 0);

  /* ------------------------------------------------------------------------ */
  button = gtk_button_new ();
  gtk_table_attach (GTK_TABLE (table), button, 0, 2, 5, 6,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y | GTK_EXPAND), 0, 0);
  
  /* these values are just a dummy size for the initial display */
  photo = gpe_create_pixmap (table, ownerphotofile, IMG_WIDTH, IMG_WIDTH);
  gtk_container_add (GTK_CONTAINER (button), photo);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (choose_photofile),
                    NULL);

  /* ------------------------------------------------------------------------ */
  /* make the labels grey: */
  gtk_rc_parse_string ("widget '*owner_name_label' style 'gpe_labels'");
  gtk_widget_set_name (owner_name_label, "owner_name_label");
  gtk_rc_parse_string ("widget '*owner_email_label' style 'gpe_labels'");
  gtk_widget_set_name (owner_email_label, "owner_email_label");
  gtk_rc_parse_string ("widget '*owner_phone_label' style 'gpe_labels'");
  gtk_widget_set_name (owner_phone_label, "owner_phone_label");
  gtk_rc_parse_string ("widget '*owner_address_label' style 'gpe_labels'");
  gtk_widget_set_name (owner_address_label, "owner_address_label");
  gtk_rc_parse_string ("widget '*owner_photofile_label' style 'gpe_labels'");
  gtk_widget_set_name (owner_photofile_label, "owner_photofile_label");
 
  /*
   * Check if we are even allowed to write to the owner data file
   */
  if (datafile_writable)
    gtk_widget_set_sensitive (table, TRUE);
  else 
    gtk_widget_set_sensitive (table, FALSE);
 
  return vbox;

}

void Ownerinfo_Free_Objects()
{
}

void Ownerinfo_Save()
{
  gchar *firstline;
  GtkTextIter pstart, pend;
	
  fp = fopen (GPE_OWNERINFO_TMP, "w");
  if (fp)
    {
      firstline = g_strdup ("[gpe-ownerinfo data version 2]");
      fputs (firstline, fp);
      fputs ("\n", fp);
	  g_free(ownerphotofile);
      ownerphotofile = g_strdup(gtk_entry_get_text (GTK_ENTRY (photofile)));
      fputs (ownerphotofile, fp);
      fputs ("\n", fp);
	  g_free(ownername);
      ownername = g_strdup(gtk_entry_get_text (GTK_ENTRY (name)));
      fputs (ownername,  fp);
      fputs ("\n", fp);
	  g_free(owneremail);
      owneremail = g_strdup(gtk_entry_get_text (GTK_ENTRY (email)));
      fputs (owneremail, fp);
      fputs ("\n", fp);
	  g_free(ownerphone);
      ownerphone = g_strdup(gtk_entry_get_text (GTK_ENTRY (phone)));
      fputs (ownerphone, fp);
      fputs ("\n", fp);
	
      gtk_text_buffer_get_start_iter(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(address)),&pstart);
      gtk_text_buffer_get_end_iter(
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(address)),&pend);
		
      owneraddress =  gtk_text_buffer_get_text(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(address)),&pstart,&pend,TRUE);
      gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(address)),owneraddress,-1);
      fputs (owneraddress, fp);
      fclose (fp);
	  suid_exec("CPOI","CPOI");
	  /* more a hack than a feature */
	  usleep(300000);
	  restart_gpe_login();
    }
  else /* fp == NULL, something went wrong */
    {
	  // don't bother our user with this
      //gpe_perror_box (GPE_OWNERINFO_DATA);
    }
}

void Ownerinfo_Restore()
{
  GtkTextIter pstart, pend;
	
  gtk_entry_set_text (GTK_ENTRY (photofile), ownerphotofile);
  gtk_entry_set_text (GTK_ENTRY (name),      ownername);
  gtk_entry_set_text (GTK_ENTRY (email),     owneremail);
  gtk_entry_set_text (GTK_ENTRY (phone),     ownerphone);

	gtk_text_buffer_get_start_iter(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(address)),&pstart);
      gtk_text_buffer_get_end_iter(
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(address)),&pend);
		
      gtk_text_buffer_delete(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(address)),&pstart,&pend);
	
}


/*
 *  Return value: The old version number found in the data file.
 *  FIXME: do this only when success, neg value when error! 
 *
 *  The data file format is described in the file HACKING.
 */

gint
maybe_upgrade_datafile ()
{
  gchar *firstline;
  guint version = 0, idx = 0;
  gint upgrade_result = UPGRADE_ERROR;
  FILE *fp;

  fp = fopen (GPE_OWNERINFO_DATA, "r");
  if (fp)
    {
      char buf[2048];
      if (fgets (buf, sizeof (buf), fp))
	{
	  firstline = g_strdup (buf);
	  firstline[strlen (firstline)-1]='\0';
	  
	  /*
	   * looking for a string like
	   *   [gpe-ownerinfo data version 42]
	   */
	  
	  if (strstr (firstline, INFO_MATCH)) { /* found magic string */
	    version = strtol (firstline + strlen (INFO_MATCH), (char **)NULL, 10);
	    
	    if (version == 0) {
	      fprintf (stderr, "gpe-conf ownerinfo: file '%s' is version %d, which should not happen.\n",
		       GPE_OWNERINFO_DATA, version);
	      fprintf (stderr, "   Please file a bug. I am continuing anyway.\n");
	    }
	    if (version > CURRENT_DATAFILE_VER) {
	      fprintf (stderr, "gpe-conf ownerinfo: file '%s' is version %d.\n   I only know how to handle version %d. Exiting.\n",
		       GPE_OWNERINFO_DATA, version, CURRENT_DATAFILE_VER);
	      fclose (fp);
	      exit (1);
	    }
	  }
	  else { /* must be version 1 which didn't have a version indicator */
	    version = 1;
	  }
	}
      fclose (fp);
      
      if (version != CURRENT_DATAFILE_VER) {
	for (idx = version+1; idx <= CURRENT_DATAFILE_VER; idx++) {
	  printf ("Need to upgrade. idx is %d\n", idx);
	  switch (idx) {
	  case 0:
	    break;
	  case 1:
	    break;
	  case 2: /* from here, it's cumulative upgrades */
	    upgrade_result = upgrade_to_v2 (idx);
	  case 3:
	    /* upgrade_result = upgrade_to_v3 (fp, idx); */ /* ...and so on */
	    break;
	  }
	}
      } else {
	upgrade_result = UPGRADE_NOT_NEEDED;
      }
    }
  else /* fp == NULL */
    { 
      perror (GPE_OWNERINFO_DATA);
    }

  if (upgrade_result == UPGRADE_ERROR) {
    printf ("Had found version %d, but cannot upgrade for some reason.\n", version);
    return (UPGRADE_ERROR);
  }
  else {
    return UPGRADE_NOT_NEEDED;
  }
}

gint
upgrade_to_v2 (guint new_version)
{
  gchar *firstline, *oldcontent;
  FILE *fp;
  
  firstline =  g_strdup ("[gpe-ownerinfo data version 2]");
  oldcontent = g_strdup ("Initial oldcontent.");
  
  fp = fopen (GPE_OWNERINFO_DATA, "r");
  if (fp)
    {
      char buf[2048];
      guint numchar;
      
      if ((numchar=fread (buf, 1, sizeof (buf), fp)))
      	{
      	  oldcontent = g_strdup (buf);
      	  oldcontent[numchar]='\0';
      	}

      printf("oldcontent:\n%s\n", oldcontent);
      
      fclose (fp);
    }
  else /* fp == NULL */
    { 
      perror (GPE_OWNERINFO_DATA);
    }
  

  fp = fopen (GPE_OWNERINFO_DATA, "w");
  if (fp)
    {
      fputs (firstline, fp);
      fputs ("\n", fp);

      fputs (PREFIX "/share/gpe/pixmaps/gpe-logo.png", fp);
      fputs ("\n", fp);

      fputs (oldcontent, fp);
      
      printf ("gpe-conf ownerinfo: Migrated data file '%s' to version %d.\n",
	      GPE_OWNERINFO_DATA, new_version);

      fclose (fp);
    }
  else /* fp == NULL */
    { 
      perror (GPE_OWNERINFO_DATA);
      return (UPGRADE_ERROR);
    }

  return (new_version);
}

static
void File_Selected (char *file,
		    gpointer data)
{
  /* check if we can read the selected file */
  /* FIXME: gotta check if it's a valid png file */
  if (access (file, R_OK) == 0) {
    gtk_entry_set_text (GTK_ENTRY (photofile), file);
    gtk_container_remove (GTK_CONTAINER (button), photo);
    photo = gpe_create_pixmap (button, file, IMG_WIDTH, IMG_HEIGHT);
    gtk_container_add (GTK_CONTAINER (button), photo);
    gtk_widget_show (GTK_WIDGET (photo));
  }
  else
    g_message ("Can't read '%s'.", file);
}

void
choose_photofile              (GtkWidget     *button,
			       gpointer       user_data)
{
  ask_user_a_file (gpe_dirname (ownerphotofile), NULL, File_Selected, NULL, NULL); 	
}
