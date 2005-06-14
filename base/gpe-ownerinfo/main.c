/*
 * Copyright (C) 2002, 2003 Colin Marquardt <ipaq@marquardt-home.de>
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

#include <X11/Xlib.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gpe/errorbox.h>
#include <gpe/init.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>
#include <gpe/stylus.h>
#include <gpe/translabel.h>

#include "gpe-ownerinfo.h"

#ifdef ENABLE_NLS
#define _(_x) gettext(_x)
#else
#define _(_x) (_x)
#endif

int
main (int argc, char *argv[])
{
  GtkWidget *GPE_Ownerinfo, *notebook;
  
  gchar * geometry = NULL;
  gboolean flag_keep_on_top = FALSE;
  gint x = -1, y = -1, h = 0, w = 0;
  gint val;
  gint opt;
  struct passwd *pwd;

  /* drop root privileges */
  pwd = getpwnam ("nobody");
  if (pwd == NULL) {
    fprintf (stderr, "getpwnam call failed. Exiting.\n");
    exit (1);
  }
  else {
    if (!seteuid (pwd->pw_uid)) {
      perror ("seteuid call failed. Exiting.");
      exit (1);
    }
  }

#ifdef ENABLE_NLS  
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
#endif

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  while ((opt = getopt (argc,argv,"hkg:")) != EOF)
    {
      switch (opt) {
      case 'g':
	geometry = optarg;
	break;
      case 'k':
	flag_keep_on_top = TRUE;
	break;
      case 'h':
	printf (_("GPE Owner Info"));
	printf ("\n");
	printf (_("Valid options:\n"));
	printf (_("   -g GEOMETRY  window geometry (default: 240x100+0+220)\n"));
	printf (_("   -k           always keep window on top (override redirect)\n"));
	printf (_("   -h           this help text\n"));
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

  /* draw the GUI */
  GPE_Ownerinfo = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (GPE_Ownerinfo, "GPE_Ownerinfo");
  gtk_object_set_data (GTK_OBJECT (GPE_Ownerinfo), "GPE_Ownerinfo", GPE_Ownerinfo);
  gtk_container_set_border_width (GTK_CONTAINER (GPE_Ownerinfo), 0);
  gtk_window_set_title (GTK_WINDOW (GPE_Ownerinfo), _("GPE Owner Info"));

  notebook = gpe_owner_info ();

  gtk_container_add (GTK_CONTAINER (GPE_Ownerinfo), notebook);

  gtk_signal_connect (GTK_OBJECT (GPE_Ownerinfo), "delete_event",
                      GTK_SIGNAL_FUNC (gtk_main_quit),
                      NULL);

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
      gtk_widget_set_usize (GPE_Ownerinfo, 240, 100);
      gtk_widget_set_uposition (GPE_Ownerinfo, 0, 220); /* 320 - 100 */
    }

  gtk_widget_realize (GPE_Ownerinfo);

  if (flag_keep_on_top) {
    gdk_window_set_override_redirect (GPE_Ownerinfo->window, TRUE);
  }

  gtk_widget_show (GPE_Ownerinfo);

  gtk_main ();
  return 0;
}
