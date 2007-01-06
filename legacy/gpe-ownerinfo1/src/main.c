/*
 * Copyright (C) 2002 Colin Marquardt <ipaq@marquardt-home.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <unistd.h> /* for access() */

#include <X11/Xlib.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gpe/errorbox.h>
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>
#include <gpe/stylus.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "rootpixmap.h"

#include "main.h"

#define CURRENT_DATAFILE_VER 2

/* WARNING: don't mess with this! */
#define GPE_OWNERINFO_DATA   "/etc/gpe/gpe-ownerinfo.data"
#define INFO_MATCH           "[gpe-ownerinfo data version "

#define UPGRADE_ERROR      -1
#define UPGRADE_NOT_NEEDED  0

GtkWidget *GPE_Ownerinfo;

gchar *ownerphotofile;

struct gpe_icon my_icons[] = {
  { "ownerphoto", "tux-48" },
  { NULL }
};

static void
mapped (GtkWidget *window)
{
  Pixmap rmap = GetRootPixmap (GDK_DISPLAY ());
  if (rmap != None)
    {
      Pixmap pmap;
      pmap = CutWinPixmap (GDK_DISPLAY(), GDK_WINDOW_XWINDOW (window->window), rmap, 
			   GDK_GC_XGC (window->style->black_gc));
      if (pmap != None)
	{
	  GdkPixmap *gpmap = gdk_pixmap_foreign_new (pmap);      
	  window->style->bg_pixmap[GTK_STATE_NORMAL] = gpmap;
	  gtk_widget_set_style (window, window->style);
	}
    }
}

int
main (int argc, char *argv[])
{

  GtkWidget *widget;
  
  gchar *ownername, *owneremail, *ownerphone, *owneraddress;
  FILE *fp;
  gchar * geometry = NULL;
  gboolean flag_transparent = FALSE;
  gboolean flag_keep_on_top = FALSE;
  gint x = -1, y = -1, h = 0, w = 0;
  gint val;
  gint opt;
  gint upgrade_result = UPGRADE_ERROR;
  
#ifdef ENABLE_NLS
  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
#endif

  while ((opt = getopt (argc,argv,"hktg:")) != EOF)
    {
      switch (opt) {
      case 'g':
	geometry = optarg;
	break;
      case 't':
	flag_transparent = TRUE;
	break;
      case 'k':
	flag_keep_on_top = TRUE;
	break;
      case 'h':
	printf ("GPE Owner Info $Revision$\n");
	printf ("\n");
	printf ("Valid options:\n");
	printf ("   -g GEOMETRY  window geometry (default: 240x120+0+200)\n");
	printf ("   -t           make window transparent\n");
	printf ("   -k           always keep window on top (override redirect)\n");
	printf ("   -h           this help text\n");
	exit (1);
      case '?':
	if (isprint (optopt))
	  ;
	/* fprintf (stderr, "gpe-ownerinfo: Unknown option -%c'.\n", optopt); */
	else
	  fprintf (stderr,
		   "gpe-ownerinfo: Unknown option character \\x%x'.\n",
		   optopt);
	break;
      default:
	fprintf (stderr,
		 "gpe-ownerinfo: Unknown error while parsing command line. Command was %c\n",
		 opt);
	exit (1);
      }
    }
    
  ownername    = g_strdup ("GPE User");
  owneremail   = g_strdup ("nobody@localhost.localdomain");
  ownerphone   = g_strdup ("+99 (9999) 999-9999");
  owneraddress = g_strdup ("The user 'root' can use\n\"GPE Configuration\" ('Owner')\nto change this data.");

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
	 *  and read the photo file name:
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
	      my_icons[0].filename = g_strdup (ownerphotofile);
	    else
	      fprintf (stderr, "gpe-ownerinfo: file '%s' could not be found, using default.\n",
		       ownerphotofile);
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

      fclose (fp);
    }
  else /* fp == NULL */
    {
      /* show default info: */
      /*
      gpe_perror_box (GPE_OWNERINFO_DATA);
      exit (1);
      */
    }

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  GPE_Ownerinfo = create_GPE_Ownerinfo ();

  if (geometry)
    {
      val = XParseGeometry (geometry, &x, &y, &w, &h);
      if ((val & (HeightValue | WidthValue)) == (HeightValue | WidthValue))
	gtk_widget_set_usize (GPE_Ownerinfo, w, h);
      if (val & (XValue | YValue))
	gtk_widget_set_uposition (GPE_Ownerinfo, x, y);
    }
  else
    {
      gtk_widget_set_usize (GPE_Ownerinfo, 240, 120);
      gtk_widget_set_uposition (GPE_Ownerinfo, 0, 200);
    }

  /* make the labels grey: */
  widget = lookup_widget (GPE_Ownerinfo, "owner_name_label");
  gtk_rc_parse_string ("widget '*owner_name_label' style 'gpe_ownerinfo_labels'");
  gtk_widget_set_name (widget, "owner_name_label");
  widget = lookup_widget (GPE_Ownerinfo, "owner_email_label");
  gtk_rc_parse_string ("widget '*owner_email_label' style 'gpe_ownerinfo_labels'");
  gtk_widget_set_name (widget, "owner_email_label");
  widget = lookup_widget (GPE_Ownerinfo, "owner_phone_label");
  gtk_rc_parse_string ("widget '*owner_phone_label' style 'gpe_ownerinfo_labels'");
  gtk_widget_set_name (widget, "owner_phone_label");
  widget = lookup_widget (GPE_Ownerinfo, "owner_address_label");
  gtk_rc_parse_string ("widget '*owner_address_label' style 'gpe_ownerinfo_labels'");
  gtk_widget_set_name (widget, "owner_address_label");
 
  widget = lookup_widget (GPE_Ownerinfo, "name");
  gtk_label_set_text (GTK_LABEL (widget), ownername);

  widget = lookup_widget (GPE_Ownerinfo, "email");
  gtk_label_set_text (GTK_LABEL (widget), owneremail);

  widget = lookup_widget (GPE_Ownerinfo, "phone");
  gtk_label_set_text (GTK_LABEL (widget), ownerphone);

  widget = lookup_widget (GPE_Ownerinfo, "address");
  gtk_label_set_text (GTK_LABEL (widget), owneraddress);

  /*
    this widget is shown in the on_smallphotobutton_size_allocate() callback
  widget = lookup_widget (GPE_Ownerinfo, "smallphoto");
  gtk_widget_show (widget);
  */

  /* make window transparent if option -t is given: */
  if (flag_transparent) {
    gtk_signal_connect (GTK_OBJECT (GPE_Ownerinfo), "map-event",
			GTK_SIGNAL_FUNC (mapped), NULL);
  }

  gtk_widget_show (GPE_Ownerinfo);

  if (flag_keep_on_top) {
    gdk_window_set_override_redirect (GPE_Ownerinfo->window, TRUE);
  }

  gtk_main ();
  return 0;
}

/*
 *  Return value: upgrade status
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
