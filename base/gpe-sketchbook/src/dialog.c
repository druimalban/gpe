/*
 * Copyright (C) 2002 luc pionchon
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>

#include "picturebutton.h"

#define _(_x) (_x) //gettext(_x)

struct __confirm_action_data{
  GtkWidget * dialog;
  void (* action_func)(gpointer data);
  gpointer action_data;
};

static void __confirm_action_dialog_box_on_action_button_clicked(GtkButton * button,
                                                                 gpointer user_data){
  struct __confirm_action_data * data = (struct __confirm_action_data *)user_data;
  data->action_func(data->action_data);
  gtk_widget_destroy(data->dialog);
}

void _confirm_action_dialog_box(gchar * text,
                                gchar * action_button_label,
                                void (*action_function)(gpointer data),
                                gpointer action_function_data){  
  
  struct __confirm_action_data action_data;

  GtkWidget * dialog;
  GtkWidget * label;
  GtkWidget * button_action;
  GtkWidget * button_cancel;

  GtkWidget * hbox;

  //--dialog
  dialog = gtk_dialog_new ();
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  gtk_widget_realize (dialog);

  //--label
  label  = gtk_label_new (text);

  //--action button
  button_action = gpe_picture_button (dialog->style, action_button_label, "ok");
  action_data.dialog      = dialog;
  action_data.action_func = action_function;
  action_data.action_data = action_function_data;
  gtk_signal_connect (GTK_OBJECT (button_action), "clicked",
                      GTK_SIGNAL_FUNC (__confirm_action_dialog_box_on_action_button_clicked), 
                      &action_data);

  //--cancel button
  button_cancel = gpe_picture_button (dialog->style, _("Cancel"), "cancel");
  gtk_signal_connect_object (GTK_OBJECT (button_cancel), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy), 
			     (gpointer)dialog);

  //--packing
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 4);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), button_cancel);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), button_action);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);

  //--show up
  gtk_widget_show_all (dialog);

  gtk_main ();
}
