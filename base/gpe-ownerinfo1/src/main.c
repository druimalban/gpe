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

#include <gtk/gtk.h>

#include "init.h"

/* Use this when libgewidget is fixed: */
/* #include <gpe/init.h> */

#include "interface.h"
#include "support.h"


int
main (int argc, char *argv[])
{
  GtkWidget *GPE_Ownerinfo;
  GtkWidget *widget;
  guint num_char = 0;

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

  GPE_Ownerinfo = create_GPE_Ownerinfo ();

  widget = lookup_widget (GPE_Ownerinfo, "name");
  gtk_label_set_text (GTK_LABEL (widget), "GPE User");

  gtk_rc_parse_string ("widget '*name' style 'gpe_username'");
  gtk_widget_set_name (widget, "name");

  widget = lookup_widget (GPE_Ownerinfo, "email");
  gtk_label_set_text (GTK_LABEL (widget), "nobody@localhost");

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
		   "c/o GPE Cabal\nGPE User's Street\nGPE User City\nGPE Userland",
		   -1);
  gtk_text_thaw (GTK_TEXT (widget));

  widget = lookup_widget (GPE_Ownerinfo, "phone");
  gtk_label_set_text (GTK_LABEL (widget), "+00 (123) 567-8900");

  
  gtk_widget_show (GPE_Ownerinfo);

  gtk_main ();
  return 0;
}
