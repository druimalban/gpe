/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>

#include "render.h"
#include "pixmaps.h"
#include "picturebutton.h"

GtkWidget *
gpe_picture_button (GtkStyle *style, gchar *text, gchar *icon)
{
  GdkPixbuf *p = gpe_find_icon (icon);
  GtkWidget *button = gtk_button_new ();
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *label = gtk_label_new (text);

  if (p)
    {
      GtkWidget *pw = gpe_render_icon (style, p);
      gtk_box_pack_start (GTK_BOX (hbox), pw, TRUE, TRUE, 0);
      gtk_widget_show (pw);
    }
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

  gtk_widget_show (hbox);
  gtk_widget_show (label);
  gtk_widget_show (button);

  gtk_container_add (GTK_CONTAINER (button), hbox);

  return button;
}
