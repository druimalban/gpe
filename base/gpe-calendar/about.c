/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <gdk_imlib.h>

#include "globals.h"

#define GPE_ICON "/usr/share/pixmaps/gpe-logo.png"

static void
thanks(GtkWidget *widget,
       GtkWidget *window)
{
  gtk_widget_destroy (window);
}

void
about(void)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_DIALOG);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GdkPixmap *gpe_pix;
  GdkBitmap *gpe_pix_mask;
  GtkWidget *label_name = gtk_label_new ("GPE-Calendar");
  GtkWidget *label_date = gtk_label_new ("version " VERSION);
  GtkWidget *label_copy = gtk_label_new ("(c) 2002 Phil Blundell");
  GtkWidget *ok = gtk_button_new_with_label ("OK");

  gtk_signal_connect (GTK_OBJECT (ok), "clicked",
		      GTK_SIGNAL_FUNC (thanks), window);

  if (gdk_imlib_load_file_to_pixmap (GPE_ICON, &gpe_pix, &gpe_pix_mask))
    {
      GtkWidget *logo = gtk_pixmap_new (gpe_pix, gpe_pix_mask);

      gtk_box_pack_start (GTK_BOX (vbox), logo, FALSE, FALSE, 0);
    }

  gtk_box_pack_start (GTK_BOX (vbox), label_name, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), label_date, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), label_copy, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), ok, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (window), vbox);
  
  gtk_widget_show_all (window);
}
