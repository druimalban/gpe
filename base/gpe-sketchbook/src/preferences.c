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
#include "gpe/gpepreferences.h"

//--own headers
#include "gpe-sketchbook.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)

void prefs_reset_defaults(){
  sketchbook.prefs.joypad_scroll  = FALSE;
  sketchbook.prefs.grow_on_scroll = TRUE;
  sketchbook.prefs.start_with     = PAGE_SELECTOR_LIST;
}

#ifdef DEBUG
void _print_prefs(){
  g_printerr("Prefs: joypad_scroll  [%s]\n", sketchbook.prefs.joypad_scroll?"on":"off");
  g_printerr("Prefs: grow_on_scroll [%s]\n", sketchbook.prefs.grow_on_scroll?"on":"off");
  g_printerr("Prefs: start_with     [%d]\n", sketchbook.prefs.start_with);
}
#endif /* DEBUG */

void prefs_fetch_settings(){
   GpePrefsResult res;
   res = gpe_prefs_get ("joypad-scroll",  G_TYPE_INT, &(sketchbook.prefs.joypad_scroll));
   res = gpe_prefs_get ("grow-on-scroll", G_TYPE_INT, &(sketchbook.prefs.grow_on_scroll));
   res = gpe_prefs_get ("start-with",     G_TYPE_INT, &(sketchbook.prefs.start_with));
}


void prefs_save_settings(){
   GpePrefsResult res;
   res = gpe_prefs_set ("joypad-scroll",  G_TYPE_INT, &(sketchbook.prefs.joypad_scroll));
   res = gpe_prefs_set ("grow-on-scroll", G_TYPE_INT, &(sketchbook.prefs.grow_on_scroll));
   res = gpe_prefs_set ("start-with",     G_TYPE_INT, &(sketchbook.prefs.start_with));
}

//-----------------------------------------------------------------------------


struct {
  GtkWidget * joypad_scroll;
  GtkWidget * grow_on_scroll;
  GtkWidget * start_with;
  gint        start_with_index;
} _prefs_ui;

void reset_gui_from_prefs(){
  // struct --> ui
  switch(sketchbook.prefs.start_with){
  //WARNING: MUST match gtk_menu_append() order!
  case PAGE_SKETCHPAD:           _prefs_ui.start_with_index = 0; break;
  case PAGE_SELECTOR_LIST:       _prefs_ui.start_with_index = 1; break;
  case PAGE_SELECTOR_ICON_TABLE: _prefs_ui.start_with_index = 2; break;
  }
  gtk_option_menu_set_history(GTK_OPTION_MENU(_prefs_ui.start_with), _prefs_ui.start_with_index);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_prefs_ui.joypad_scroll), sketchbook.prefs.joypad_scroll);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_prefs_ui.grow_on_scroll), sketchbook.prefs.grow_on_scroll);
}

void reset_prefs_from_gui(){
  // ui -> struct
  switch(gtk_option_menu_get_history(GTK_OPTION_MENU(_prefs_ui.start_with))){
  //WARNING: MUST match gtk_menu_append() order!
  case 0: sketchbook.prefs.start_with = PAGE_SKETCHPAD;           break;
  case 1: sketchbook.prefs.start_with = PAGE_SELECTOR_LIST;       break;
  case 2: sketchbook.prefs.start_with = PAGE_SELECTOR_ICON_TABLE; break;
  }

  sketchbook.prefs.joypad_scroll = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_prefs_ui.joypad_scroll));
  sketchbook.prefs.grow_on_scroll = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_prefs_ui.grow_on_scroll));
}

void on_button_ok_clicked (GtkButton *button, gpointer _unused){
  reset_prefs_from_gui();
  //**/_print_prefs();
  gtk_notebook_set_page(sketchbook.notebook, PAGE_SELECTOR);
}
void on_button_no_clicked (GtkButton *button, gpointer _unused){
  reset_gui_from_prefs();
  //**/_print_prefs();
  gtk_notebook_set_page(sketchbook.notebook, PAGE_SELECTOR);
}


GtkWidget * preferences_gui(GtkWidget * window){
  GtkWidget * vbox;
  GtkWidget * separator;
  GtkWidget * hbox;
  GtkWidget * button_no;
  GtkWidget * button_ok;

  vbox = gtk_vbox_new(FALSE, 4);

  //--Preferences

  {//start with
    GtkWidget * label;
    GtkWidget * option_menu;
    GtkWidget * menu;
    GtkWidget * menu_item;
    GtkWidget * hbox;
    label = gtk_label_new(_("Start with"));

    menu = gtk_menu_new();
    menu_item = gtk_menu_item_new_with_label(_("Sketchpad"));
    gtk_menu_append(GTK_MENU(menu), menu_item);
    menu_item = gtk_menu_item_new_with_label(_("Selector - list"));
    gtk_menu_append(GTK_MENU(menu), menu_item);
    menu_item = gtk_menu_item_new_with_label(_("Selector - icons table"));
    gtk_menu_append(GTK_MENU(menu), menu_item);

    option_menu = gtk_option_menu_new();
    gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
    
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(hbox), label,        FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(hbox), option_menu,  TRUE, TRUE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox,  FALSE, FALSE, 4);

    _prefs_ui.start_with = option_menu;
  }

  {//joypad scroll
    GtkWidget * check;
    check = gtk_check_button_new_with_label(_("Use joypad scrolling"));
    gtk_box_pack_start(GTK_BOX(vbox), check,      FALSE, FALSE, 4);
    _prefs_ui.joypad_scroll = check;
  }

  {//grow on scroll
    GtkWidget * check;
    check = gtk_check_button_new_with_label(_("Grow on scroll"));
    gtk_box_pack_start(GTK_BOX(vbox), check,      FALSE, FALSE, 4);
    _prefs_ui.grow_on_scroll = check;
  }

  //--init gui
  reset_gui_from_prefs();

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
