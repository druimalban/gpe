/*
 * Copyright (C) 2002, 2003 Colin Marquardt <ipaq@marquardt-home.de>
 *               2005 Florian Boor <florian@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#endif
#include <sys/types.h> /* for getpwnam() */
#include <pwd.h>       /* for getpwnam() */
#include <unistd.h>    /* for access() */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/errorbox.h>
#include <gpe/init.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>
#include <gpe/translabel.h>

#include "gpe-ownerinfo.h"

#define CURRENT_DATAFILE_VER 2

/* WARNING: don't mess with this! */
#define GPE_OWNERINFO_DATA   "/etc/gpe/gpe-ownerinfo.data"
#define INFO_MATCH           "[gpe-ownerinfo data version "

#define UPGRADE_ERROR      -1
#define UPGRADE_NOT_NEEDED  0

#ifdef ENABLE_NLS
#define _(_x) gettext(_x)
#else
#define _(_x) (_x)
#endif

static GtkWidget *name;
static GtkWidget *phone;
static GtkWidget *email;
static GtkWidget *address;
static GdkPixbuf *photopixbuf = NULL;
static GtkWidget *smallphotobutton;
static GtkWidget *bigphotobutton;
static gchar *photofile = PREFIX "/share/pixmaps/gpe/gpe-logo.png";

/* redraw the pixbuf */
static gboolean
on_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  gint width  = 10;
  gint height = 10;
  guint maxwidth = 32, maxheight = 32;
  guint resultwidth, resultheight;
  gfloat scale, scale_width = 2.72, scale_height = 3.14;
  GdkPixbuf *scaledpixbuf;

  if (!photopixbuf)
    return TRUE;

  maxwidth  = widget->allocation.width;
  maxheight = widget->allocation.height;

  width  = gdk_pixbuf_get_width (photopixbuf);
  height = gdk_pixbuf_get_height (photopixbuf);

  if (width > maxwidth)
    scale_width = (gfloat) maxwidth / width;
  else
    scale_width = 1.0;

  if (height > maxheight)
    scale_height = (gfloat) maxheight / height;
  else
    scale_height = 1.0;

  scale = scale_width < scale_height ? scale_width : scale_height;

  resultwidth  = (gint) (width  * scale);
  resultheight = (gint) (height * scale);

  scaledpixbuf = gdk_pixbuf_scale_simple (photopixbuf, resultwidth, resultheight,
					  GDK_INTERP_BILINEAR);

  gdk_pixbuf_render_to_drawable_alpha (scaledpixbuf, widget->window,
				       0, 0,                      /* src_x, src_y */
				       (gint) ((maxwidth - resultwidth)/2),   /* dest_x */
				       (gint) ((maxheight - resultheight)/2), /* dest_y */
				       -1, -1,                    /* use the pixbuf size */
				       GDK_PIXBUF_ALPHA_BILEVEL,  /* ignored */
				       128,                       /* ignored */
				       GDK_RGB_DITHER_NORMAL,     /* dither mode */
				       0, 0);                     /* x_dither, y_dither */
  
  return TRUE;
}

static void
on_smallphotobutton_clicked            (GtkButton       *button,
                                        GtkWidget       *notebook)
{
  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 1);
}

static void
on_bigphotobutton_clicked              (GtkButton       *button,
                                        GtkWidget       *notebook)
{
  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 0);
}

/*
 *  Return value: upgrade status
 *
 *  The data file format is described in the file HACKING.
 */

static gint
upgrade_to_v2 (guint new_version)
{
  gchar *firstline, *oldcontent;
  FILE *fp;
  
  firstline = g_strdup ("Initial firstline, must be looooooooooooooooooong"
			" enough for the later content");
  sprintf (firstline, "%s %d]", INFO_MATCH, new_version);
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

      printf ("gpe-ownerinfo: oldcontent:\n%s\n", oldcontent);
      
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

      fputs (PREFIX "/share/pixmaps/gpe/default/tux-48.png", fp);
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


static gint
maybe_upgrade_datafile ()
{
  gchar *firstline;
  guint version = 0, idx = 0;
  gint upgrade_result = UPGRADE_ERROR;
  FILE *fp;

  return UPGRADE_NOT_NEEDED;
	
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
	      fprintf (stderr, "gpe-ownerinfo: file '%s' is version %d,"
		       " which should not happen.\n",
		       GPE_OWNERINFO_DATA, version);
	      fprintf (stderr, "   Please file a bug. I am continuing anyway.\n");
	    }
	    if (version > CURRENT_DATAFILE_VER) {
	      fprintf (stderr, "gpe-ownerinfo: file '%s' is version %d.\n"
		       "   I only know how to handle version %d. Exiting.\n",
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
	  printf ("gpe-ownerinfo: Need to upgrade. idx is %d\n", idx);
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
    printf ("gpe-ownerinfo: Had found version %d,"
	    " but cannot upgrade for some reason.\n", version);
    return (UPGRADE_ERROR);
  }
  else {
    /* printf ("gpe-ownerinfo: found version %d, which is the current one.\n", version); */
    return UPGRADE_NOT_NEEDED;
  }
}

static void
translate_name_label (GtkWidget *namelabel, gpointer data)
{
  gtk_label_set_markup (GTK_LABEL (namelabel),
			/* cheat a bit with the space here*/
			g_strdup_printf ("<b>%s</b> ",
					 /* TRANSLATORS: only a short word will look good
					  here.  If the word is shorter than 5 letters,
					  please fill it up with spaces.*/
					 _("Owner")));
}

GtkWidget *
gpe_owner_info (void)
{
  GtkWidget *notebook;
  GtkWidget *mainvbox;
  GtkWidget *cathbox;
  GtkWidget *indentedhbox;
  GtkWidget *leftcolvbox;
  GtkWidget *namelabel;
  GtkWidget *address_button_vbox;
  GtkWidget *smallphotodrawingarea;
  GtkWidget *bigphotodrawingarea;
  GtkWidget *rightcolvbox;
  GtkSizeGroup *sizegroup;
  GtkWidget *sv, *vp;
  GError *error = NULL;
  gint upgrade_result = UPGRADE_ERROR;
  
  /* gchar *gpe_catindent = gpe_get_catindent ();  */
  /* guint gpe_catspacing = gpe_get_catspacing (); */
  guint gpe_boxspacing = gpe_get_boxspacing ();
  /* guint gpe_border     = gpe_get_border ();     */

  gchar *ownername, *owneremail, *ownerphone, *owneraddress;
  gchar *ownerphotofile;
  FILE *fp;

  ownername    = g_strdup (_("GPE User"));
  /* TRANSLATORS: you should make sure to not use a 'fantasy' domain
     which might actually exist (maybe in the future). You can
     replace the 'nobody' though, or use '@example.org' (see RFC 2606) */
  owneremail   = g_strdup (_("nobody@localhost.net"));
  ownerphone   = g_strdup (_("+99 (9999) 999-9999"));
  /* TRANSLATORS: the translations below should match 'Owner
     Information' (in gpe-conf) and 'Settings' (in mbdesktop) */
  owneraddress = g_strdup (_("Configurable with <i>Owner Information</i>"
			     " under <i>Settings</i>."));

  fp = fopen (GPE_OWNERINFO_DATA, "r");
  if (fp)
    {
      char buf[2048];
      guint numchar;
      gchar *firstline;

      upgrade_result = maybe_upgrade_datafile ();

   
      if (upgrade_result == UPGRADE_NOT_NEEDED) {	
	/*  we have at least datafile version 2, so we need to skip
	 *  the 1st line and read the photo file name:
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
	      photofile = g_strdup (ownerphotofile);
	    else
	      fprintf (stderr, "gpe-ownerinfo: file '%s' could not be found,\n"
		       "   using default file '%s'.\n",
		       ownerphotofile, photofile);
	  }
      }
      else {
	/* upgrade went wrong, need to handle old format */
	/* Note: handle all possible old formats here */
      }
      /* now get the rest of the data: */
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
	  else 
		owneraddress = g_strdup("");

      fclose (fp);
    }

  photopixbuf = gdk_pixbuf_new_from_file (photofile, &error);
  if (!photopixbuf)
    {
      fprintf (stderr, "gpe-ownerinfo: failed to open icon '%s' (%s),",
               photofile, error->message);
      g_error_free(error);
    }

  /* notebook with two pages;
   * page 1 holds the small photo and the info text,
   * page 2 just the big photo:
   */
  notebook = gtk_notebook_new ();
  GTK_WIDGET_UNSET_FLAGS (notebook, GTK_CAN_FOCUS);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);

  /*
   * The first notebook page
   */
  mainvbox = gtk_vbox_new (FALSE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), mainvbox, NULL);

  cathbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (mainvbox), cathbox, TRUE, TRUE, 0);

  indentedhbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (indentedhbox);
  gtk_box_pack_start (GTK_BOX (cathbox), indentedhbox, TRUE, TRUE, 0);

  leftcolvbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (indentedhbox), leftcolvbox,
		      FALSE, FALSE, gpe_boxspacing);

  namelabel = gtk_label_new (NULL);
  gtk_widget_add_translation_hook (namelabel, translate_name_label, NULL);
  gtk_box_pack_start (GTK_BOX (leftcolvbox), namelabel, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (namelabel), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (namelabel), 0, 0.5);

  address_button_vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (leftcolvbox), address_button_vbox, TRUE, TRUE, 0);

  smallphotobutton = gtk_button_new ();
  GTK_WIDGET_UNSET_FLAGS (smallphotobutton, GTK_CAN_FOCUS);
  gtk_button_set_relief (GTK_BUTTON (smallphotobutton), GTK_RELIEF_NONE);
  gtk_box_pack_start (GTK_BOX (address_button_vbox), smallphotobutton, TRUE, TRUE, 0);

  smallphotodrawingarea = gtk_drawing_area_new ();
  gtk_widget_add_events (GTK_WIDGET (smallphotodrawingarea),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  g_signal_connect (G_OBJECT (smallphotodrawingarea), "expose_event",  
                    G_CALLBACK (on_expose_event), NULL);
  gtk_container_add (GTK_CONTAINER (smallphotobutton), smallphotodrawingarea);
  
  rightcolvbox = gtk_vbox_new (FALSE, gpe_boxspacing);

  name = gtk_label_new (ownername);
  gtk_box_pack_start (GTK_BOX (rightcolvbox), name, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (name), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (name), 0, 0.5);

  email = gtk_label_new (owneremail);
  gtk_box_pack_start (GTK_BOX (rightcolvbox), email, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (email), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (email), 0, 0.5);

  phone = gtk_label_new (ownerphone);
  gtk_box_pack_start (GTK_BOX (rightcolvbox), phone, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (phone), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (phone), 0, 0.5);

  address = gtk_label_new(owneraddress);
  gtk_label_set_justify (GTK_LABEL (address), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (address), 0, 0.5);
  gtk_label_set_line_wrap(GTK_LABEL(address), TRUE);
  gtk_box_pack_start (GTK_BOX (rightcolvbox), address, FALSE, TRUE, 0);
  
  sv = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sv), GTK_POLICY_NEVER, 
                                 GTK_POLICY_AUTOMATIC);
  vp = gtk_viewport_new(NULL, NULL);
  gtk_viewport_set_shadow_type(GTK_VIEWPORT(vp), GTK_SHADOW_NONE);  
  gtk_container_add(GTK_CONTAINER(sv), vp);
  gtk_container_add(GTK_CONTAINER(vp), rightcolvbox);
  gtk_box_pack_start (GTK_BOX (indentedhbox), sv, TRUE, TRUE, 0);

  /*
   * The second notebook page
   */
  bigphotobutton = gtk_button_new ();
  gtk_widget_show (bigphotobutton);
  GTK_WIDGET_UNSET_FLAGS (bigphotobutton, GTK_CAN_FOCUS);
  gtk_button_set_relief (GTK_BUTTON (bigphotobutton), GTK_RELIEF_NONE);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), bigphotobutton, NULL);

  bigphotodrawingarea = gtk_drawing_area_new ();
  gtk_widget_add_events (GTK_WIDGET (bigphotodrawingarea),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  g_signal_connect (G_OBJECT (bigphotodrawingarea), "expose_event",  
                    G_CALLBACK (on_expose_event), NULL);
  gtk_widget_show (bigphotodrawingarea);
  gtk_container_add (GTK_CONTAINER (bigphotobutton), bigphotodrawingarea);
  
  /* Make sure the labels are all the same height (which might not
   * be the case because of the pango markup for e.g. 'email').
   * Don't put 'address' in it, it is too high.
   */
  sizegroup = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget (sizegroup, namelabel);
  gtk_size_group_add_widget (sizegroup, name);
  gtk_size_group_add_widget (sizegroup, email);
  gtk_size_group_add_widget (sizegroup, phone);

  /* end of drawing the GUI */

  gtk_label_set_markup (GTK_LABEL (name),
			g_strdup_printf ("<span><b>%s</b></span>",
					 ownername));
  gtk_label_set_selectable (GTK_LABEL (name), TRUE);
  /* maybe play with <span stretch='ultracondensed'> here, but we
     likely don't have the fonts: */
  gtk_label_set_markup (GTK_LABEL (email),
			g_strdup_printf ("<span foreground='darkblue'>%s</span>",
					 owneremail));
  gtk_label_set_selectable (GTK_LABEL (email), TRUE);
  gtk_label_set_markup (GTK_LABEL (phone),
			g_strdup_printf ("<span>%s</span>",
					 ownerphone));
  gtk_label_set_selectable (GTK_LABEL (phone), TRUE);
    
  gtk_label_set_markup (GTK_LABEL (address),
                        g_strdup_printf ("<span>%s</span>",
					    owneraddress));
                     
  gtk_signal_connect (GTK_OBJECT (smallphotobutton), "clicked",
		      GTK_SIGNAL_FUNC (on_smallphotobutton_clicked),
		      notebook);
  gtk_signal_connect (GTK_OBJECT (bigphotobutton), "clicked",
		      GTK_SIGNAL_FUNC (on_bigphotobutton_clicked),
		      notebook);

  gtk_widget_set_usize(notebook, -1, 80);
  gtk_widget_show_all(notebook);
  gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);
  return notebook;
}
