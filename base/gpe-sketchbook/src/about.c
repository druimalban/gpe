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
#include "pixmaps.h"
#include "render.h"

#include "picturebutton.h"

#define _(_x) (_x) //gettext(_x)

void about_box(gchar * app_name,
               gchar * app_version, //optional, may be NULL
               gchar * app_icon,
               gchar * app_short_description,
               gchar * minihelp_text,
               gchar * legal_text){
  
  GtkWidget * dialog;

  GdkPixbuf * icon_pixbuf;
  GtkWidget * icon_gpe;
  GtkWidget * icon_app;
  GtkWidget * label_name;
  GtkWidget * label_version;

  GtkWidget * label_description;

  GtkWidget * label_minihelp;
  GtkWidget * label_legal;

  GtkWidget * button_ok;

  gchar * s;

  //--dialog
  dialog = gtk_dialog_new ();
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  s = g_strconcat(_("About "), app_name, NULL);//FIXME: this is bad localization!
  gtk_window_set_title (GTK_WINDOW (dialog), s);
  g_free(s);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  gtk_widget_realize (dialog);

  //--about header: [icon_gpe] name/version [icon_app]
  icon_pixbuf = gpe_find_icon ("../gpe-logo48");//FIXME: this is BAD! (../)
  icon_gpe    = gpe_render_icon (GTK_DIALOG (dialog)->vbox->style, icon_pixbuf);

  icon_pixbuf = gpe_find_icon (app_icon);
  icon_app    = gpe_render_icon (GTK_DIALOG (dialog)->vbox->style, icon_pixbuf);

  label_name = gtk_label_new (app_name);

  {//try to give it a big bold font
    GdkFont  * font_bold;
    font_bold = gdk_font_load ("-*-helvetica-bold-r-normal--*-100-*-*-*-*-iso8859-1");
    if(font_bold){
      GtkStyle * style;
      style = gtk_widget_get_style(label_name);
      gdk_font_unref(style->font);
      style->font = font_bold;
      gtk_widget_set_style(label_name, style);
    }
  }

  label_version = gtk_label_new(app_version);//creates an empty one if NULL string


  //--short description
  label_description = gtk_label_new (app_short_description);
  {//try to give it an italic font
    //FIXME: it overwrites the label_name font (...)

    //GdkFont  * font_it;
    //font_it = gdk_font_load ("-*-*-medium-i-*--*-80-*-*-*-*-iso8859-1");
    //if(font_it){
    //  GtkStyle * style;
    //  style = gtk_widget_get_style(label_description);
    //  gdk_font_unref(style->font);
    //  style->font = font_it;
    //  gtk_widget_set_style(label_description, style);
    //}
  }

  //--minihelp and legal
  label_minihelp = gtk_label_new (minihelp_text);
  label_legal    = gtk_label_new (legal_text);

  gtk_label_set_justify  (GTK_LABEL(label_minihelp), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap(GTK_LABEL(label_minihelp), TRUE);
  gtk_label_set_line_wrap(GTK_LABEL(label_legal),    TRUE);
  //FIXME: give them a white background (don't forget the separator)

  //--button
  button_ok = gpe_picture_button (dialog->style, _("OK"), "ok");
  gtk_signal_connect_object (GTK_OBJECT (button_ok), "clicked",
                             GTK_SIGNAL_FUNC (gtk_widget_destroy), 
                             (gpointer)dialog);

  //--packing
  {
    GtkWidget * vbox;            // all
    GtkWidget * hbox;            // icon labels icons
    GtkWidget * vbox2;           // name/version labels
    GtkWidget * scrolled_window; // minihelp legal
    GtkWidget * vbox3;           // minihelp/legal 
    GtkWidget * empty_label;     //FIXME: use GtkTable or GtkFixed or...

    //all "about" info in vbox
    vbox  = gtk_vbox_new(FALSE, 5);
    gtk_box_set_spacing(GTK_BOX(vbox), 5);
    gtk_container_set_border_width (GTK_CONTAINER (vbox),  5);
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
    
    //header and short descr.
    vbox2 = gtk_vbox_new (TRUE, 5);
    empty_label = gtk_label_new(NULL);
    gtk_box_pack_start (GTK_BOX (vbox2), empty_label,    FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox2), label_name,     TRUE, TRUE,   0);
    gtk_box_pack_start (GTK_BOX (vbox2), label_version,  TRUE, TRUE,   0);

    hbox  = gtk_hbox_new (FALSE, 5);
    gtk_box_pack_start (GTK_BOX (hbox), icon_gpe,  FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vbox2,     TRUE,  TRUE,  0);
    gtk_box_pack_end   (GTK_BOX (hbox), icon_app,  FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    
    gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new(), FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), label_description,    FALSE, FALSE, 0);
    
    //mini help and legal
    vbox3 = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width (GTK_CONTAINER (vbox3),  5);
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), vbox3);
    gtk_box_pack_start (GTK_BOX (vbox),  scrolled_window, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox3), label_minihelp,  TRUE, TRUE, 0);
    gtk_box_pack_end   (GTK_BOX (vbox3), label_legal,     FALSE, FALSE, 0);
    gtk_box_pack_end   (GTK_BOX (vbox3), gtk_hseparator_new(), FALSE, FALSE, 0);
    
    //OK button
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), button_ok);
  }
  //--show up
  gtk_widget_show_all (dialog);

  gtk_main ();
}
