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
#include "gpe/init.h"
#include "gpe/pixmaps.h"

//--own headers
#include "gpe-sketchbook.h"
#include "sketchpad.h"
#include "sketchpad-gui.h"
#include "selector.h"
#include "selector-gui.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)

gchar * sketchdir;
#define SKETCHPAD_HOME_DIR ".sketchbook"

static struct gpe_icon my_icons[] = {
  //Apps icon
  { "this_app_icon", PREFIX "/share/pixmaps/gpe-sketchbook.png" },
  { "gpe-logo", PREFIX "/share/gpe/pixmaps/gpe-logo.png" },

  //GPE stock icons
  { "new",    "new"   },
  { "open",   "open"  },
  { "delete", "delete"},
  { "save",   "save"  },
  { "left",      "left"},
  { "right",    "right"},
  { "about",  "about" },
  { "list",      "gpe-sketchbook/list"},
  { "sketchpad", "gpe-sketchbook/sketchpad" },

  //own icons
  { "tool_pencil",  "gpe-sketchbook/tool_pencil"},
  { "tool_eraser",  "gpe-sketchbook/tool_eraser"},
  { "brush_small",  "gpe-sketchbook/brush_small"},
  { "brush_medium", "gpe-sketchbook/brush_medium"},
  { "brush_large",  "gpe-sketchbook/brush_large"},
  { "brush_xlarge", "gpe-sketchbook/brush_xlarge"},

  { NULL, NULL }
};

void app_init(int argc, char ** argv);
void app_about();
void gui_init();

int main (int argc, char ** argv){

  if (gpe_application_init (&argc, &argv) == FALSE) exit (1);
  if (gpe_load_icons (my_icons) == FALSE) exit (1);

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
  setlocale (LC_ALL, "");

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
  GdkPixmap *pixmap;
  GdkBitmap *bitmap;

  window_selector = create_window_selector();
  window_selector_init(window_selector);
  gtk_window_set_title (GTK_WINDOW (window_selector), _("Sketch selector"));

  window_sketchpad = sketchpad_build_window();
  window_sketchpad_init(window_sketchpad);
  //gtk_window_set_title (GTK_WINDOW (window_sketchpad), "Sketch: new");

  gtk_widget_realize   (window_selector);
  gtk_widget_realize   (window_sketchpad);
  if (gpe_find_icon_pixmap ("this_app_icon", &pixmap, &bitmap)){
    gdk_window_set_icon (window_selector->window,  NULL, pixmap, bitmap);
    gdk_window_set_icon (window_sketchpad->window, NULL, pixmap, bitmap);
  }

#ifdef DESKTOP //FIXME: to move into related -gui.c
  gtk_window_set_position (GTK_WINDOW (window_selector),  GTK_WIN_POS_CENTER);
  gtk_window_set_position (GTK_WINDOW (window_sketchpad), GTK_WIN_POS_CENTER);
#endif

  //the window to show first
  //NOTE: could be defined by preference, or command line argument

  //if(1){ workaround...
  //  //gtk_widget_realize   (window_sketchpad);
  //  gtk_widget_show   (window_sketchpad);
  //  gtk_widget_hide   (window_sketchpad);
  //  gtk_widget_show   (window_selector);
  //}
  //else  gtk_widget_show   (window_sketchpad);

  if(1) gtk_widget_show   (window_sketchpad);
  else  gtk_widget_show   (window_selector);

}//gui_init()
