/*
 * Copyright (C) 2002 luc pionchon
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gpe/pixmaps.h"
#include "gpe/render.h"
#include "gpe/picturebutton.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)

gboolean user_choice;
#define ACTION TRUE
#define CANCEL FALSE

static void _confirm_action(GtkButton * button, gpointer dialog_to_destroy){
  user_choice = ACTION;
  gtk_widget_destroy(GTK_WIDGET(dialog_to_destroy));
}

gboolean confirm_action_dialog_box(gchar * text,
                                   gchar * action_button_label){  

  GtkWidget * dialog;

  GtkWidget * hbox;
  GtkWidget * icon;
  GtkWidget * label;

  GtkWidget * button_action;
  GtkWidget * button_cancel;


  //--by default, cancel the action
  user_choice = CANCEL;

  //--dialog
  dialog = gtk_dialog_new ();
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  gtk_widget_realize (dialog);

  //--warning icon
  {
    GdkPixbuf * icon_pixbuf;
    icon_pixbuf = gpe_find_icon ("question");
    icon = gpe_render_icon (GTK_DIALOG (dialog)->vbox->style, icon_pixbuf);
  }

  //--label
  label = gtk_label_new (text);

  //--action button
  button_action = gpe_picture_button (dialog->style, action_button_label, "ok");
  gtk_signal_connect (GTK_OBJECT (button_action), "clicked",
                      GTK_SIGNAL_FUNC (_confirm_action),
                      (gpointer)dialog);

  //--cancel button
  button_cancel = gpe_picture_button (dialog->style, _("Cancel"), "cancel");
  gtk_signal_connect_object (GTK_OBJECT (button_cancel), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy), 
			     (gpointer)dialog);

  //--packing
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), icon,  TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), button_cancel);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), button_action);

  //--show up
  gtk_widget_show_all (dialog);

  gtk_main ();

  return user_choice;
}//confirm_action_dialog_box()
