/*
 * Copyright (C) 2002 Damien Tanner <dctanner@magenet.com>
 * Copyright (C) 2002 luc pionchon
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <libintl.h>
#include <stdio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "render.h"
#include "picturebutton.h"
#include "pixmaps.h"

#define _(x) dgettext(PACKAGE, x)

guint window_x = 240, window_y = 310;

void
gpe_about (gchar * app_name,
               gchar * app_version, //optional, may be NULL
               gchar * app_icon,
               gchar * app_short_description,
               gchar * minihelp_text,
               gchar * legal_text)
{
  
  GtkWidget * window_about;

  GdkPixbuf * icon_pixbuf;
  GtkWidget * icon_gpe;
  GtkWidget * icon_app;
  GtkWidget * label_name;
  GtkWidget * label_version;

  GtkWidget * label_description;

  GtkWidget * event_box;//to set the labels background
  GtkWidget * label_minihelp;
  GtkWidget * label_legal;

  GtkWidget * button_ok;

  gchar * s;

  //--window_about
  window_about = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_modal (GTK_WINDOW (window_about), TRUE);//NOTE: modal?
  //gtk_window_set_position (GTK_WINDOW (window_about), GTK_WIN_POS_CENTER);
  s = g_strconcat(_("About "), app_name, NULL);//FIXME: this is bad localization!
  gtk_window_set_title (GTK_WINDOW (window_about), s);
  g_free(s);
  gtk_signal_connect (GTK_OBJECT (window_about), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  gtk_widget_realize (window_about);

  //--about header: [icon_gpe] name/version [icon_app]
  icon_pixbuf = gpe_find_icon ("gpe-logo");
  icon_gpe    = gpe_render_icon (window_about->style, icon_pixbuf);

  icon_pixbuf = gpe_find_icon (app_icon);
  icon_app    = gpe_render_icon (window_about->style, icon_pixbuf);

  label_name = gtk_label_new (app_name);

  {//try to give it a big bold font
    GdkFont  * font_bold;
    font_bold = gdk_font_load ("-*-helvetica-bold-r-normal--*-100-*-*-*-*-iso8859-1");
    if(font_bold){
      label_name->style = gtk_style_copy(label_name->style);
      label_name->style->font = font_bold;
      gtk_widget_set_style(label_name, label_name->style);//for some reason this is needed...
    }
  }

  label_version = gtk_label_new(app_version);//this creates an empty one if NULL string

  //--short description
  label_description = gtk_label_new (app_short_description);
  gtk_label_set_line_wrap(GTK_LABEL(label_description), TRUE);
  {//try to give it an italic font
    GdkFont  * font_it;
    font_it = gdk_font_load ("-*-*-medium-i-*--*-100-*-*-*-*-iso8859-1");
    if(font_it){
      label_description->style = gtk_style_copy(label_description->style);
      label_description->style->font = font_it;
      gtk_widget_set_style(label_description, label_description->style);
    }
  }

  //--minihelp and legal
  label_minihelp = gtk_label_new (minihelp_text);
  label_legal    = gtk_label_new (legal_text);

  gtk_label_set_justify  (GTK_LABEL(label_minihelp), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap(GTK_LABEL(label_minihelp), TRUE);
  gtk_label_set_line_wrap(GTK_LABEL(label_legal),    TRUE);
  
  event_box = gtk_event_box_new();
  {//give them a white background
    //FIXME: use gtkrc style, to anable the user to set his/her preference.
    GtkStyle * style;
    style = gtk_style_copy(event_box->style);
    style->bg[0] = style->white;
    style->bg[1] = style->white;
    style->bg[2] = style->white;
    style->bg[3] = style->white;
    style->bg[4] = style->white;
    gtk_widget_set_style(event_box, style);
  }

  //--button
  button_ok = gpe_picture_button (window_about->style, _("OK"), "ok");
  gtk_signal_connect_object (GTK_OBJECT (button_ok), "clicked",
                             GTK_SIGNAL_FUNC (gtk_widget_destroy), 
                             (gpointer)window_about);

  //--packing
  {
    GtkWidget * main_vbox;       // all
    GtkWidget * vbox;            // all info
    GtkWidget * hbox;            // icon labels icons
    GtkWidget * vbox2;           // name/version labels
    GtkWidget * scrolled_window; // minihelp legal
    GtkWidget * vbox3;           // minihelp/legal 
    GtkWidget * empty_label;     // to make app_name "centered"

    //all "about" info in vbox
    vbox  = gtk_vbox_new(FALSE, 5);
    gtk_box_set_spacing(GTK_BOX(vbox), 5);
    gtk_container_set_border_width (GTK_CONTAINER (vbox),  5);
    
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

    gtk_box_pack_start (GTK_BOX (vbox), hbox,                 FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new(), FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), label_description,    FALSE, FALSE, 0);
    
    //mini help and legal
    vbox3 = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width (GTK_CONTAINER (vbox3),  5);
    gtk_box_pack_start (GTK_BOX (vbox3), label_minihelp,       TRUE, TRUE, 0);
    gtk_box_pack_end   (GTK_BOX (vbox3), label_legal,          FALSE, FALSE, 0);
    gtk_box_pack_end   (GTK_BOX (vbox3), gtk_hseparator_new(), FALSE, FALSE, 0);

    gtk_container_add  (GTK_CONTAINER (event_box),  vbox3);

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), event_box);
    gtk_box_pack_start (GTK_BOX (vbox),  scrolled_window, TRUE, TRUE, 0);

    //button OK
    hbox  = gtk_hbox_new (FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
    gtk_box_pack_start (GTK_BOX (hbox), button_ok, TRUE, TRUE, 0);
    
    //info, separator, button OK
    main_vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start (GTK_BOX (main_vbox), vbox,                 TRUE,  TRUE, 0);
    gtk_box_pack_start (GTK_BOX (main_vbox), gtk_hseparator_new(), FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (main_vbox), hbox,                 FALSE, FALSE, 0);
    gtk_container_add  (GTK_CONTAINER (window_about), main_vbox);

  }//end packing

  //--show up
  gtk_widget_show_all (window_about);

  gtk_main ();
}
