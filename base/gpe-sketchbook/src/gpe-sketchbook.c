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

#ifndef PREFIX
#  define PREFIX "/usr"
#endif

#include <gtk/gtk.h>

//--GPE libs
#include "init.h"
#include "pixmaps.h"

//--own headers
#include "gpe-sketchbook.h"
#include "sketchpad.h"
#include "sketchpad-gui.h"
#include "selector.h"
#include "selector-gui.h"

gchar * sketchdir;
#define SKETCHPAD_HOME_DIR ".sketchbook"

static struct gpe_icon my_icons[] = {
  //Apps icon
  { "this_app_icon", PREFIX "/share/pixmaps/gpe-sketchbook.png" },

  //GPE stock icons
  { "new",    "new"   },
  { "open",   "open"  },
  { "delete", "delete"},
  { "save",   "save"  },
  //might be moved to GPE stock
  { "prev",      PREFIX "/share/gpe-sketchbook/pixmaps/gprev.png"},
  { "next",      PREFIX "/share/gpe-sketchbook/pixmaps/gnext.png"},
  { "about",     PREFIX "/share/gpe-sketchbook/pixmaps/about.png" },
  { "list",      PREFIX "/share/gpe-sketchbook/pixmaps/glist.png"},
  { "sketchpad", PREFIX "/share/gpe-sketchbook/pixmaps/sketchpad.png" },

  //own icons
  { "pencil",       PREFIX "/share/gpe-sketchbook/pixmaps/tool_pencil.png"},
  { "eraser",       PREFIX "/share/gpe-sketchbook/pixmaps/tool_eraser.png"},
  { "brush_small",  PREFIX "/share/gpe-sketchbook/pixmaps/brush_small.png"},
  { "brush_medium", PREFIX "/share/gpe-sketchbook/pixmaps/brush_medium.png"},
  { "brush_large",  PREFIX "/share/gpe-sketchbook/pixmaps/brush_large.png"},
  { "brush_xlarge", PREFIX "/share/gpe-sketchbook/pixmaps/brush_xlarge.png"},
  { "color_black",  PREFIX "/share/gpe-sketchbook/pixmaps/color_black.png"},
  { "color_red",    PREFIX "/share/gpe-sketchbook/pixmaps/color_red.png"},
  { "color_green",  PREFIX "/share/gpe-sketchbook/pixmaps/color_green.png"},
  { "color_blue",   PREFIX "/share/gpe-sketchbook/pixmaps/color_blue.png"},

  { NULL, NULL }
};

void app_init(int argc, char ** argv);
void app_about();
void gui_init();

int main (int argc, char ** argv){

  if (gpe_application_init (&argc, &argv) == FALSE) exit (1);

//  setlocale (LC_ALL, "");
//  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
//  textdomain (PACKAGE);

  if (gpe_load_icons (my_icons) == FALSE) exit (1);

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
  //**/g_print("%s\n", ABOUT_TEXT);
}//app_quit()

void app_quit(){
  //NOTE: may ask for unsaved sketch
  gtk_exit (0);
}//app_quit()

void gui_init(){

  window_selector = create_window_selector();
  window_selector_init(window_selector);
  gtk_window_set_title (GTK_WINDOW (window_selector), "Sketch selector");

  window_sketchpad = sketchpad_build_window();
  window_sketchpad_init(window_sketchpad);
  //gtk_window_set_title (GTK_WINDOW (window_sketchpad), "Sketch: new");

  //the window to show first
  //NOTE: could be defined by preference, or command line argument
  if(1) gtk_widget_show   (window_selector);
  else  gtk_widget_show   (window_sketchpad);

}//gui_init()
