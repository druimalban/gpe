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
#include <string.h>
#include <getopt.h>
#include <ctype.h>

#include <X11/Xlib.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <gpe/errorbox.h>
#include <gpe/init.h>

#include "interface.h"
#include "support.h"

#define GPE_OWNERINFO_DATA "/etc/gpe/gpe-ownerinfo.data"

int
main (int argc, char *argv[])
{
  GtkWidget *GPE_Ownerinfo;
  GtkWidget *widget;
  gchar *ownername, *owneremail, *ownerphone, *owneraddress;
  FILE *fp;
  gchar * geometry = NULL;
  int x = -1, y = -1, h = 0, w = 0;
  int val;
  int opt;

#ifdef ENABLE_NLS
  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
#endif

  gtk_set_locale ();

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  add_pixmap_directory (PACKAGE_DATA_DIR "/pixmaps");
  add_pixmap_directory (PACKAGE_SOURCE_DIR "/pixmaps");

  while ((opt = getopt (argc,argv,"hg:")) != EOF)
    {
      switch (opt) {
      case 'g':
	geometry = optarg;
	break;
      case 'h':
	printf ("GPE Owner Info $Revision\n");
	printf ("\n");
	printf ("Valid options: -g GEOMETRY   window geometry (default: 240x120+0+200)\n");
	printf ("               -h            this help text\n");
	exit(1);
      case '?':
	if (isprint (optopt))
	  fprintf (stderr, "gpe-ownerinfo: Unknown option -%c'.\n", optopt);
	else
	  fprintf (stderr,
		   "gpe-ownerinfo: Unknown option character \\x%x'.\n",
		   optopt);
	break;
      default:
	fprintf (stderr,
		 "gpe-ownerinfo: Unknown error while parsing command line. Command was %c\n",
		 opt);
	exit(1);
      }
    }
    
  ownername    = g_strdup ("GPE User");
  owneremail   = g_strdup ("nobody@localhost.localdomain");
  ownerphone   = g_strdup ("+99 (9999) 999-9999");
  owneraddress = g_strdup ("Edit file\n/etc/gpe/gpe-ownerinfo.data\nto change this data.");

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
  else /* fp == NULL */
    {
      gpe_perror_box (GPE_OWNERINFO_DATA);
      exit (1);
    }

  widget = lookup_widget (GPE_Ownerinfo, "name");
  gtk_label_set_text (GTK_LABEL (widget), ownername);

  gtk_rc_parse_string ("widget '*name' style 'gpe_username'");
  gtk_widget_set_name (widget, "name");

  widget = lookup_widget (GPE_Ownerinfo, "email");
  gtk_label_set_text (GTK_LABEL (widget), owneremail);

  widget = lookup_widget (GPE_Ownerinfo, "address");
  gtk_label_set_text (GTK_LABEL (widget), owneraddress);

  widget = lookup_widget (GPE_Ownerinfo, "phone");
  gtk_label_set_text (GTK_LABEL (widget), ownerphone);

  widget = lookup_widget (GPE_Ownerinfo, "frame1");
  gtk_rc_parse_string ("widget '*gpe_ownerinfo' style 'gpe_ownerinfo_bg'");
  gtk_widget_set_name (widget, "gpe_ownerinfo");

  gtk_widget_show (GPE_Ownerinfo);

  gtk_main ();
  return 0;
}
