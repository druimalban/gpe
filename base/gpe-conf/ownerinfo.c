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

#include <gpe/errorbox.h>

#include "applets.h"
#include "ownerinfo.h"

#define GPE_OWNERINFO_DATA "/etc/gpe/gpe-ownerinfo.data"

GtkWidget *name;
GtkWidget *email;
GtkWidget *phone;
GtkWidget *address;

static gchar *ownername, *owneremail, *ownerphone, *owneraddress;

FILE *fp;

GtkWidget *Ownerinfo_Build_Objects()
{  
  GtkWidget *table1;
  GtkWidget *owner_name_label;
  GtkWidget *owner_email_label;
  GtkWidget *owner_phone_label;
  GtkWidget *owner_address_label;
  GtkWidget *scrolledwindow1;
  GtkWidget *viewport1;

  /* ======================================================================== */
  /* get the data from the file if existant */
  ownername    = g_strdup ("GPE User");
  owneremail   = g_strdup ("nobody@localhost.localdomain");
  ownerphone   = g_strdup ("+99 (9999) 999-9999");
  owneraddress = g_strdup ("Data stored in\n/etc/gpe/gpe-ownerinfo.data.");

  fp = fopen (GPE_OWNERINFO_DATA, "r");
  if (fp)
    {
      char buf[2048];
      guint numchar;
      
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
  table1 = gtk_table_new (4, 2, FALSE);
  gtk_widget_set_name (table1, "table1");
  gtk_widget_show (table1);

  /* ------------------------------------------------------------------------ */
  owner_name_label = gtk_label_new (_("Name"));
  gtk_table_attach (GTK_TABLE (table1), owner_name_label, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (owner_name_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (owner_name_label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (owner_name_label), 5, 0);

  name = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (name), ownername);
  gtk_table_attach (GTK_TABLE (table1), name, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  /* ------------------------------------------------------------------------ */
  owner_email_label = gtk_label_new (_("E-Mail"));
  gtk_table_attach (GTK_TABLE (table1), owner_email_label, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (owner_email_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (owner_email_label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (owner_email_label), 5, 0);

  email = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (email), owneremail);
  gtk_table_attach (GTK_TABLE (table1), email, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  /* ------------------------------------------------------------------------ */
  owner_phone_label = gtk_label_new (_("Phone"));
  gtk_table_attach (GTK_TABLE (table1), owner_phone_label, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (owner_phone_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (owner_phone_label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (owner_phone_label), 5, 0);

  phone = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (phone), ownerphone);
  gtk_table_attach (GTK_TABLE (table1), phone, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  /* ------------------------------------------------------------------------ */
  owner_address_label = gtk_label_new (_("Address"));
  gtk_table_attach (GTK_TABLE (table1), owner_address_label, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_label_set_justify (GTK_LABEL (owner_address_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (owner_address_label), 0, 0);
  gtk_misc_set_padding (GTK_MISC (owner_address_label), 5, 0);


  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_table_attach (GTK_TABLE (table1), scrolledwindow1, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
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

  return table1;

}

void Ownerinfo_Free_Objects()
{
}

void Ownerinfo_Save()
{
  fp = fopen (GPE_OWNERINFO_DATA, "w");
  if (fp)
    {
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
  gtk_entry_set_text (GTK_ENTRY (name),  ownername);
  gtk_entry_set_text (GTK_ENTRY (email), owneremail);
  gtk_entry_set_text (GTK_ENTRY (phone), ownerphone);

  gtk_editable_delete_text (GTK_EDITABLE (address), 0, -1);
  gtk_text_freeze (GTK_TEXT (address));
  gtk_text_insert (GTK_TEXT (address), NULL, &address->style->black, NULL,
                   owneraddress, -1);
  gtk_text_thaw (GTK_TEXT (address));
}
