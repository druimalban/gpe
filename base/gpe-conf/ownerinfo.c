/*
 * ownerinfo module for gpe-conf
 *
 * Copyright (C) 2002 Colin Marquardt <ipaq@marquardt-home.de>
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
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#define _XOPEN_SOURCE /* For GlibC2 */
#include <time.h>
/* #include <libgen.h> */ /* for dirname() */

#include <gpe/errorbox.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>

#include "applets.h"
#include "ownerinfo.h"

#define CURRENT_DATAFILE_VER 2

/* WARNING: don't mess with this! */
#define GPE_OWNERINFO_DATA   "/etc/gpe/gpe-ownerinfo.data"
#define INFO_MATCH           "[gpe-ownerinfo data version "

#define UPGRADE_ERROR      -1
#define UPGRADE_NOT_NEEDED  0

GtkWidget *photofile;
GtkWidget *name;
GtkWidget *email;
GtkWidget *phone;
GtkWidget *address;

static gchar *ownerphotofile, *ownername, *owneremail, *ownerphone, *owneraddress;

FILE *fp;

GtkWidget *Ownerinfo_Build_Objects()
{
  GtkWidget *table;
  GtkWidget *owner_photofile_label;
  GtkWidget *owner_name_label;
  GtkWidget *owner_email_label;
  GtkWidget *owner_phone_label;
  GtkWidget *owner_address_label;
  GtkWidget *scrolledwindow1;
  GtkWidget *viewport1;
  gint upgrade_result = UPGRADE_ERROR;

  GtkWidget *button;
  GtkWidget *photo;

  /* ======================================================================== */
  /* get the data from the file if existant */
  ownerphotofile = g_strdup (PREFIX "/share/gpe/pixmaps/default/tux-48.png");
  ownername      = g_strdup ("GPE User");
  owneremail     = g_strdup ("nobody@localhost.localdomain");
  ownerphone     = g_strdup ("+99 (9999) 999-9999");
  owneraddress   = g_strdup ("Data stored in\n/etc/gpe/gpe-ownerinfo.data.");

  fp = fopen (GPE_OWNERINFO_DATA, "r");
  if (fp)
    {
      char buf[2048];
      guint numchar;
      gchar *firstline;

      upgrade_result = maybe_upgrade_datafile ();

      printf ("upgrade_result: %d\n", upgrade_result);
      
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
	    my_icons[9].filename = g_strdup (ownerphotofile);
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
      	  owneraddress = g_strdup (buf);
      	  owneraddress[numchar]='\0';
      	}

      fclose (fp);
    }
  else /* fp == NULL, no file existing */
    {
      /* it's just okay if the file doesn't exist */
      /*
	gpe_perror_box (GPE_OWNERINFO_DATA);
	exit (1);
      */
    }

  /* ======================================================================== */
  /* draw the GUI */
  table = gtk_table_new (6, 2, FALSE);
  gtk_widget_set_name (table, "table");
  gtk_widget_show (table);

  /* ------------------------------------------------------------------------ */
  owner_name_label = gtk_label_new (_("Name"));
  gtk_table_attach (GTK_TABLE (table), owner_name_label, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (owner_name_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (owner_name_label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (owner_name_label), 5, 0);

  name = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (name), ownername);
  gtk_table_attach (GTK_TABLE (table), name, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  /* ------------------------------------------------------------------------ */
  owner_email_label = gtk_label_new (_("E-Mail"));
  gtk_table_attach (GTK_TABLE (table), owner_email_label, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (owner_email_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (owner_email_label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (owner_email_label), 5, 0);

  email = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (email), owneremail);
  gtk_table_attach (GTK_TABLE (table), email, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  /* ------------------------------------------------------------------------ */
  owner_phone_label = gtk_label_new (_("Phone"));
  gtk_table_attach (GTK_TABLE (table), owner_phone_label, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (owner_phone_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (owner_phone_label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (owner_phone_label), 5, 0);

  phone = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (phone), ownerphone);
  gtk_table_attach (GTK_TABLE (table), phone, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  /* ------------------------------------------------------------------------ */
  owner_address_label = gtk_label_new (_("Address"));
  gtk_table_attach (GTK_TABLE (table), owner_address_label, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_label_set_justify (GTK_LABEL (owner_address_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (owner_address_label), 0, 0);
  gtk_misc_set_padding (GTK_MISC (owner_address_label), 5, 0);


  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_table_attach (GTK_TABLE (table), scrolledwindow1, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  viewport1 = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), viewport1);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport1), GTK_SHADOW_NONE);

  address = gtk_text_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (viewport1), address);
  gtk_text_set_editable (GTK_TEXT (address), TRUE);
  gtk_text_insert (GTK_TEXT (address), NULL, NULL, NULL,
                   owneraddress, strlen(owneraddress));

  /* ------------------------------------------------------------------------ */
  owner_photofile_label = gtk_label_new (_("Photofile"));
  gtk_table_attach (GTK_TABLE (table), owner_photofile_label, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_label_set_justify (GTK_LABEL (owner_photofile_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (owner_photofile_label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (owner_photofile_label), 5, 0);

  photofile = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (photofile), ownerphotofile);

  gtk_table_attach (GTK_TABLE (table), photofile, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  button = gtk_button_new ();
  gtk_table_attach (GTK_TABLE (table), button, 0, 2, 5, 6,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  

  photo = create_pixmap (table, ownerphotofile);
  gtk_container_add (GTK_CONTAINER (button), photo);

  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (choose_photofile),
                      NULL);

  /*
   * Check if we are even allowed to write to the owner data file
   *    or create it if necessary.
   * FIXME:
   *    Gotta do the same at top level of gpe-conf since the buttons
   *    are not insensitive otherwise.
   */

  /* either we can write to the file... */
  if ((access (GPE_OWNERINFO_DATA, W_OK) == 0) ||
      /* ...or we are allowed to write in this directory, and the file does not yet exist */
      (((access (my_dirname (g_strdup(GPE_OWNERINFO_DATA)), W_OK)) == 0) &&
       (access (GPE_OWNERINFO_DATA, F_OK) != 0)))
    gtk_widget_set_sensitive (table, TRUE);
  else 
    gtk_widget_set_sensitive (table, FALSE);
 
  return table;

}

void Ownerinfo_Free_Objects()
{
}

void Ownerinfo_Save()
{
  gchar *firstline;
  
  fp = fopen (GPE_OWNERINFO_DATA, "w");
  if (fp)
    {
      firstline = g_strdup ("[gpe-ownerinfo data version 2]");
      fputs (firstline, fp);
      fputs ("\n", fp);
      ownerphotofile = gtk_entry_get_text (GTK_ENTRY (photofile));
      fputs (ownerphotofile, fp);
      fputs ("\n", fp);
      ownername = gtk_entry_get_text (GTK_ENTRY (name));
      fputs (ownername,  fp);
      fputs ("\n", fp);
      owneremail = gtk_entry_get_text (GTK_ENTRY (email));
      fputs (owneremail, fp);
      fputs ("\n", fp);
      ownerphone = gtk_entry_get_text (GTK_ENTRY (phone));
      fputs (ownerphone, fp);
      fputs ("\n", fp);
      owneraddress = gtk_editable_get_chars (GTK_EDITABLE (address), 0, -1);
      fputs (owneraddress, fp);
      fclose (fp);
    }
  else /* fp == NULL, something went wrong */
    {
      gpe_perror_box (GPE_OWNERINFO_DATA);
      /* don't exit, it might be "just" permission denied: */
      /* exit (1); */
    }
}

void Ownerinfo_Restore()
{
  gtk_entry_set_text (GTK_ENTRY (photofile),  ownerphotofile);
  gtk_entry_set_text (GTK_ENTRY (name),   ownername);
  gtk_entry_set_text (GTK_ENTRY (email),  owneremail);
  gtk_entry_set_text (GTK_ENTRY (phone),  ownerphone);

  gtk_editable_delete_text (GTK_EDITABLE (address), 0, -1);
  gtk_text_freeze (GTK_TEXT (address));
  gtk_text_insert (GTK_TEXT (address), NULL, &address->style->black, NULL,
                   owneraddress, -1);
  gtk_text_thaw (GTK_TEXT (address));
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
	      fprintf (stderr, "gpe-ownerinfo: file '%s' is version %d, which should not happen.\n",
		       GPE_OWNERINFO_DATA, version);
	      fprintf (stderr, "   Please file a bug. I am continuing anyway.\n");
	    }
	    if (version > CURRENT_DATAFILE_VER) {
	      fprintf (stderr, "gpe-ownerinfo: file '%s' is version %d.\n   I only know how to handle version %d. Exiting.\n",
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
    printf ("Had found version %d.\n", version);
    return UPGRADE_NOT_NEEDED;
  }
}

gint
upgrade_to_v2 (guint new_version)
{
  gchar *firstline, *oldcontent;
  FILE *fp;
  
  /* firstline = g_strdup ("Initial firstline"); */
#warning FIXME: Why doesnt this work?
  /* sprintf (firstline, "%s %d]", INFO_MATCH, new_version); */
  /* sprintf (firstline, INFO_MATCH "%d]", INFO_MATCH, new_version); maybe??? */
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

      fputs (PREFIX "/share/gpe/pixmaps/default/tux-48.png", fp);
      fputs ("\n", fp);

      fputs (oldcontent, fp);
      
      printf ("gpe-ownerinfo: Migrated data file '%s' to version %d.\n",
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
  if (access (file, R_OK) == 0) 
    gtk_entry_set_text (GTK_ENTRY (photofile), file);
  else
    g_message ("Can't read '%s'.", file);
}

void
choose_photofile              (GtkWidget     *button,
			       gpointer       user_data)
{
  ask_user_a_file (my_dirname (ownerphotofile), NULL, File_Selected, NULL, NULL);
#warning FIXME: update the image in the button here  
  /* gtk_widget_draw (GTK_WIDGET (button), NULL); */
}

/* This is an internally used function to create pixmaps. */
GtkWidget*
create_pixmap                          (GtkWidget       *widget,
                                        const gchar     *filename)
{
  GtkWidget *pixmap;
  GdkPixbuf *icon;
  
  gint width, height;
  gfloat scale, scale_width = 2.72, scale_height = 3.14;

  const guint maxwidth = 80, maxheight = 80;
  
  icon = gpe_find_icon (filename);
  
  width  = gdk_pixbuf_get_width (icon);
  height = gdk_pixbuf_get_height (icon);

  /* g_message ("image is %d x %d", width, height); */

  if (width > maxwidth)
    scale_width = (gfloat) maxwidth / width;
  else
    scale_width = 1.0;

  if (height > maxheight)
    scale_height = (gfloat) maxheight / height;
  else
    scale_height = 1.0;

  /* g_message ("scale_width: %f, scale_height: %f", scale_width, scale_height); */

  scale = scale_width < scale_height ? scale_width : scale_height;

  /* g_message ("scale: %f", scale); */
  
  pixmap = gpe_render_icon (widget->style,
				gdk_pixbuf_scale_simple
				(icon, width * scale, height * scale, GDK_INTERP_BILINEAR));
  
  /* gdk_pixbuf_unref(icon); FIXME: needed? gives trouble... */

  return pixmap;  
}

/* MacOS X doesn't have a dirname... */
char
*my_dirname (char *s) {
  int i;
  for (i=strlen (s); i && s[i]!='/'; i--);
  s[i] = 0;
  return s;
}

char
*my_basename (char *s) {
  int i;
  for (i=strlen (s); i && s[i]!='/'; i--);
  return s+i+1;
}
