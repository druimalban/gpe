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



//------------------------------------------------------------ XSettings
#define KEY_BASE "GPE/SKETCHBOOK"

#include <gpe/errorbox.h>

#include <gdk/gdkx.h> //GDK_DISPLAY

#include "xsettings-common.h"
#include "xsettings-client.h"

void _fetch_bool(XSettingsClient * client, const char * key, gboolean * bool){
  XSettingsSetting * setting;
  gchar * name;
  name = g_strdup_printf("%s/%s", KEY_BASE, key);
  if (xsettings_client_get_setting (client, name, &setting) == XSETTINGS_SUCCESS){
    if(setting->type == XSETTINGS_TYPE_INT){
      * bool = (setting->data.v_int)?TRUE:FALSE;
      /**/g_printerr("%s, set to: %d\n", name, setting->data.v_int);
    }
    xsettings_setting_free(setting);
  }
  else /**/g_printerr("no %s\n", name);
  g_free(name);
}

void prefs_fetch_settings(){
  Display * dpy = GDK_DISPLAY ();
  XSettingsClient  * client;

  //--Setup client
  client = xsettings_client_new (dpy, DefaultScreen (dpy), NULL, NULL, NULL);
  if (client == NULL){
      gpe_error_box (_("Cannot create XSettings client.\nUse default preferences."));
      return;
  }

  //--Retrieve values
  _fetch_bool(client, "joypad-scroll",  &(sketchbook.prefs.joypad_scroll));
  _fetch_bool(client, "grow-on-scroll", &(sketchbook.prefs.grow_on_scroll));

  //--Close client
  xsettings_client_destroy (client);
}

void _write_setting_int(Display * dpy, Window win,
                        Atom gpe_settings_update_atom,
                        Window manager,
                        const char * key,
                        const gint value){

  XSettingsType type;
  size_t length = 0;
  size_t name_len;
  gchar *buffer;
  XClientMessageEvent ev;
  gboolean done = FALSE;
  gchar * name;

  name = g_strdup_printf("%s/%s", KEY_BASE, key);

  type = XSETTINGS_TYPE_INT;
  length = 4;

  name_len = strlen (name);
  name_len = (name_len + 3) & ~3;
  buffer = g_malloc (length + 4 + name_len);
  *buffer = type;
  buffer[1] = 0;
  buffer[2] = name_len & 0xff;
  buffer[3] = (name_len >> 8) & 0xff;
  memcpy (buffer + 4, name, name_len);
  
  *((unsigned long *)(buffer + 4 + name_len)) = value;

  XChangeProperty (dpy, win, gpe_settings_update_atom, gpe_settings_update_atom,
                   8, PropModeReplace, buffer, length + 4 + name_len);
      
  g_free (buffer);

  XSelectInput (dpy, win, PropertyChangeMask);
      
  ev.type = ClientMessage;
  ev.window = win;
  ev.message_type = gpe_settings_update_atom;
  ev.format = 32;
  ev.data.l[0] = gpe_settings_update_atom;
  XSendEvent (dpy, manager, FALSE, 0, (XEvent *)&ev);
  
  while (! done){
    XEvent ev;
    XNextEvent (dpy, &ev);
    switch (ev.xany.type){
    case PropertyNotify:
      if (ev.xproperty.window == win
          && ev.xproperty.atom == gpe_settings_update_atom)
        done = TRUE;
      break;
    }
  }
}

void prefs_save_settings(){
  Display * dpy = GDK_DISPLAY();
  Atom gpe_settings_update_atom = XInternAtom (dpy, "_GPE_SETTINGS_UPDATE", 0);
  Window manager = XGetSelectionOwner (dpy, gpe_settings_update_atom);
  Window win;

  if (manager == None){
    gpe_error_box( _("gpe-confd is not running.\nCannot save the preferences."));
    return;
  }
  win = XCreateSimpleWindow (dpy, DefaultRootWindow (dpy), 1, 1, 1, 1, 0, 0, 0);// ???

  _write_setting_int(dpy, win, gpe_settings_update_atom, manager,
                     "joypad-scroll",  (sketchbook.prefs.joypad_scroll)?1:0);
  _write_setting_int(dpy, win, gpe_settings_update_atom, manager,
                     "grow-on-scroll", (sketchbook.prefs.grow_on_scroll)?1:0);

}

//-----------------------------------------------------------------------------


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
  prefs_save_settings();//FIXME: should be done only when exit application
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
