/* gpe-sketchbook -- a sketches notebook program for PDA
 * Copyright (C) 2002 Luc Pionchon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <gtk/gtk.h>

//--GPE libs
#include "gpe/picturebutton.h"

//--own headers
#include "gpe-sketchbook.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)

void prefs_reset_defaults(){
  sketchbook.prefs.joypad_scroll  = FALSE;
  sketchbook.prefs.grow_on_scroll = TRUE;
}

#ifdef DEBUG
void _print_prefs(){
  g_printerr("Prefs: joypad_scroll  [%s]\n", sketchbook.prefs.joypad_scroll?"on":"off");
  g_printerr("Prefs: grow_on_scroll [%s]\n", sketchbook.prefs.grow_on_scroll?"on":"off");
}
#endif /* DEBUG */


struct {
  GtkWidget * joypad_scroll;
  GtkWidget * grow_on_scroll;
} _prefs_ui;

void reset_gui_from_prefs(){
  // struct --> ui
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_prefs_ui.joypad_scroll), sketchbook.prefs.joypad_scroll);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_prefs_ui.grow_on_scroll), sketchbook.prefs.grow_on_scroll);
}
void reset_prefs_from_gui(){
  // ui -> struct
  sketchbook.prefs.joypad_scroll = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_prefs_ui.joypad_scroll));

  sketchbook.prefs.grow_on_scroll = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_prefs_ui.grow_on_scroll));

}

void on_button_ok_clicked (GtkButton *button, gpointer _unused){
  reset_prefs_from_gui();
  gtk_notebook_set_page(sketchbook.notebook, PAGE_SELECTOR);
  //**/_print_prefs();
}
void on_button_no_clicked (GtkButton *button, gpointer _unused){
  reset_gui_from_prefs();
  gtk_notebook_set_page(sketchbook.notebook, PAGE_SELECTOR);
  //**/_print_prefs();
}

GtkWidget * preferences_gui(GtkWidget * window){
  GtkWidget * vbox;
  GtkWidget * separator;
  GtkWidget * hbox;
  GtkWidget * button_no;
  GtkWidget * button_ok;

  vbox = gtk_vbox_new(FALSE, 4);

  //--Preferences
  {//joypad scroll
    GtkWidget * check;
    check = gtk_check_button_new_with_label(_("Use joypad scrolling"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), sketchbook.prefs.joypad_scroll);
    gtk_box_pack_start(GTK_BOX(vbox), check,      FALSE, FALSE, 4);
    _prefs_ui.joypad_scroll = check;
  }

  {//grow on scroll
    GtkWidget * check;
    check = gtk_check_button_new_with_label(_("Grow on scroll"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), sketchbook.prefs.grow_on_scroll);
    gtk_box_pack_start(GTK_BOX(vbox), check,      FALSE, FALSE, 4);
    _prefs_ui.grow_on_scroll = check;
  }

  //--Buttons
  button_ok = gpe_picture_button(window->style , _("OK")    , "!gtk-ok");
  button_no = gpe_picture_button(window->style , _("Cancel"), "!gtk-no");
  g_signal_connect (G_OBJECT (button_ok), "clicked", G_CALLBACK (on_button_ok_clicked), NULL);
  g_signal_connect (G_OBJECT (button_no), "clicked", G_CALLBACK (on_button_no_clicked), NULL);

  hbox = gtk_hbox_new(TRUE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), button_no, TRUE, TRUE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), button_ok, TRUE, TRUE, 4);

  separator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox,      FALSE, FALSE, 0);

  gtk_widget_show_all(vbox);
  return vbox;
}
