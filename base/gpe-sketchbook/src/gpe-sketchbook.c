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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "gpe-sketchbook.h"
#include "sketchpad.h"
#include "selector.h"
#include "_interface.h"
#include "_support.h"

gchar * sketchdir;

GtkWidget *window_about;
GtkWidget *window_dialog;
gint     dialog_action;

#define SKETCHPAD_HOME_DIR ".sketchbook"
#define ABOUT_TEXT \
        "sketchbook\n"\
        "a sketchbook program for PDA,\n"\
        "distributed under GPL"

void app_init(int argc, char ** argv);
void app_about();
void gui_init();

int main (int argc, char ** argv){

#ifdef ENABLE_NLS
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
#endif
  gtk_set_locale();

  /* ... */
  add_pixmap_directory (PACKAGE_DATA_DIR   "/pixmaps");
  /**/add_pixmap_directory (PACKAGE_SOURCE_DIR "/pixmaps");

  gtk_init (&argc, &argv);
  app_init (argc, argv);
  gui_init ();
  gtk_main ();

  return 0;
}//main()

void app_init(int argc, char ** argv){
  sketchdir = g_strdup_printf("%s/%s/", g_get_home_dir(), SKETCHPAD_HOME_DIR);
  selector_init();
  sketchpad_init();
}

void app_about(){
  /**/g_print("%s\n", ABOUT_TEXT);
}//app_quit()

void app_quit(){
  //NOTE: may ask for unsaved sketch
  gtk_exit (0);
}//app_quit()

void gui_init(){
  GtkWidget * label_about;


  window_selector = create_window_selector();
  window_selector_init(window_selector);
  gtk_window_set_title (GTK_WINDOW (window_selector), "Sketch selector");

  window_sketchpad = create_window_sketchpad();
  window_sketchpad_init(window_sketchpad);
  //gtk_window_set_title (GTK_WINDOW (window_sketchpad), "Sketch: new");

  //the window to show first
  //NOTE: could be defined by preference, or command line argument
  if(1) gtk_widget_show   (window_selector);
  else  gtk_widget_show   (window_sketchpad);

  //--about
  window_about = create_window_about();
  gtk_window_set_title (GTK_WINDOW (window_about), "About Sketch");
  label_about = lookup_widget(window_about, "label_about_text");
  gtk_label_set_text((GtkLabel *)label_about, ABOUT_TEXT);

  //--generic dialog
  window_dialog = create_window_dialog();
  gtk_window_set_title (GTK_WINDOW (window_dialog), "Sketch - dialog");

}//gui_init()

void dialog_set_text(gchar * text){
  GtkWidget * label_dialog;
  label_dialog = lookup_widget(window_dialog, "label_dialog_text");
  gtk_label_set_text((GtkLabel *)label_dialog, text);
}
