/*
 * Copyright (C) 2002 luc pionchon
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

//NOTE: make a pretty one, (look at gnome's one)
//NOTE: may add GPE icon by default + App icon (param)

#include <gtk/gtk.h>

#include "picturebutton.h"

#define _(_x) (_x) //gettext(_x)

void _about_box(gchar * app_name,
                gchar * app_version,
              //gchar * app_icon,
                gchar * app_description,
                gchar * legal){
  
  GtkWidget * dialog;

  GtkWidget * frame;
  GtkWidget * vbox;
  GtkWidget * label_name;
  GtkWidget * label_description;
  GtkWidget * label_legal;

  GtkWidget * button;

  gchar * s;

  //--dialog
  dialog = gtk_dialog_new ();
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  s = g_strconcat("About ", app_name, NULL);
  gtk_window_set_title (GTK_WINDOW (dialog), s);
  g_free(s);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  gtk_widget_realize (dialog);

  //--about...
  s = g_strconcat(app_name, " ", app_version, NULL);
  label_name        = gtk_label_new (s);
  label_description = gtk_label_new (app_description);
  label_legal       = gtk_label_new (legal);
  g_free(s);

  //--button
  button = gpe_picture_button (dialog->style, _("OK"), "ok");
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                             GTK_SIGNAL_FUNC (gtk_widget_destroy), 
                             (gpointer)dialog);

  //--packing
  frame = gtk_frame_new(NULL);
  vbox  = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  gtk_container_add (GTK_CONTAINER (vbox), label_name);
  gtk_container_add (GTK_CONTAINER (vbox), gtk_hseparator_new());
  gtk_container_add (GTK_CONTAINER (vbox), label_description);
  gtk_container_add (GTK_CONTAINER (vbox), gtk_hseparator_new());
  gtk_container_add (GTK_CONTAINER (vbox), label_legal);

  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), frame);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), button);

  //--show up
  gtk_widget_show_all (dialog);

  gtk_main ();
}
