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

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "errorbox.h"
#include "init.h"

/* Use this when libgewidget is fixed: */
/* #include <gpe/init.h> */

#include "interface.h"
#include "support.h"

#define GPE_OWNERINFO_DATA "/etc/gpe/gpe-ownerinfo.data"
#define GPE_OWNERINFO_GEOM "/etc/X11/gpe-ownerinfo.geometry"

int
main (int argc, char *argv[])
{
  GtkWidget *GPE_Ownerinfo;
  GtkWidget *widget;
  guint num_char = 0;

  gchar *ownername, *owneremail, *ownerphone, *owneraddress;
  FILE *fp;

  gboolean geometry_set = FALSE;
  
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

  ownername    = g_strdup("GPE User");
  owneremail   = g_strdup("nobody@localhost.localdomain");
  ownerphone   = g_strdup("+99 (9999) 999-9999");
  owneraddress = g_strdup("Edit file\n/etc/X11/gpe-ownerinfo.geometry\nto change this data.");

  GPE_Ownerinfo = create_GPE_Ownerinfo ();

  fp = fopen (GPE_OWNERINFO_GEOM, "r");
  if (fp)
    {
      char buf[1024];
      if (fgets (buf, sizeof (buf), fp))
	{
	  int x = -1, y = -1, h, w;
	  int val;
	  buf[strlen(buf)-1] = 0;
	  val = XParseGeometry (buf, &x, &y, &w, &h);
	  if ((val & (HeightValue | WidthValue)) == (HeightValue | WidthValue))
	    {
	      gtk_widget_set_usize (GPE_Ownerinfo, w, h);
	      geometry_set = TRUE;
	    }
	  if (val & (XValue | YValue))
	    gtk_widget_set_uposition (GPE_Ownerinfo, x, y);
	}
      fclose (fp);
    }
  
  if (geometry_set == FALSE)
    {
      /* If no window manager is running, set size to full-screen */
      /*
	<mallum> pb: request the 'SubstructureRedirect' event mask on the root window
	<mallum> pb: if that fails, theres already a window manager running
      */
      GdkEventMask ev = gdk_window_get_events (GPE_Ownerinfo->window);
      int r;
      gdk_error_trap_push ();
      XSelectInput (GDK_WINDOW_XDISPLAY (GPE_Ownerinfo->window),
		    RootWindow (GDK_WINDOW_XDISPLAY (GPE_Ownerinfo->window), 0),
		    SubstructureRedirectMask);
      gdk_flush ();
      r = gdk_error_trap_pop ();
      gdk_window_set_events (GPE_Ownerinfo->window, ev);
      if (r == 0)
	{
	  gtk_widget_set_usize (GPE_Ownerinfo, gdk_screen_width (), gdk_screen_height ());
	  geometry_set = TRUE;
	}
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
      	  owneraddress[numchar-1]='\0';
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
  num_char = gtk_text_get_length (GTK_TEXT (widget));
  if( num_char > 0 )
  {
    gtk_text_freeze (GTK_TEXT (widget));
    gtk_text_backward_delete(GTK_TEXT(widget), num_char);
    gtk_text_thaw (GTK_TEXT (widget));
  }

  gtk_text_freeze (GTK_TEXT (widget));
  gtk_text_insert (GTK_TEXT (widget), NULL, &widget->style->black, NULL,
                   owneraddress,
                   -1);
  gtk_text_thaw (GTK_TEXT (widget));

  widget = lookup_widget (GPE_Ownerinfo, "phone");
  gtk_label_set_text (GTK_LABEL (widget), ownerphone);

  
  gtk_widget_show (GPE_Ownerinfo);

  gtk_main ();
  return 0;
}
