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
#include "sketchpad-cb.h" //on_window_sketchpad_destroy()
#include "selector.h"
#include "selector-gui.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)


Sketchbook sketchbook;

#define SKETCHPAD_HOME_DIR ".sketchbook"

static struct gpe_icon my_icons[] = {
  //Apps icon
  { "this_app_icon", PREFIX "/share/pixmaps/gpe-sketchbook.png" },
  { "gpe-logo",      PREFIX "/share/gpe/pixmaps/gpe-logo.png" },

  //GPE stock icons
  { "new",    "new"   },
  { "open",   "edit"  },
  { "delete", "delete"},
  { "save",   "save"  },
  { "properties",   "properties"  },
  { "left",   "left"  },
  { "right",  "right" },

  { "list",      "gpe-sketchbook/view-mode-list"},
  { "icons",     "gpe-sketchbook/view-mode-icons"},
  //{ "sketchpad", "gpe-sketchbook/view-mode-sketchpad" },
  { "files",     "gpe-sketchbook/file-entry"},

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
  sketchbook.save_dir = g_strdup_printf("%s/%s/", g_get_home_dir(), SKETCHPAD_HOME_DIR);
  selector_init();
  sketchpad_init();
}

void app_quit(){
  gtk_exit (0);
}

void sketchbook_reset_title(GtkNotebook * notebook,
                            GtkNotebookPage * page,
                            guint page_number,
                            gpointer unused){
  switch (page_number){
  case PAGE_SKETCHPAD:
    sketchpad_reset_title();
    break;
  case PAGE_SELECTOR:
  default:
    gtk_window_set_title (GTK_WINDOW (sketchbook.window), _("Sketchbook"));
  }
}

void gui_init(){
  GdkPixmap *pixmap;
  GdkBitmap *bitmap;

  GtkWidget * window;
  GtkNotebook * notebook;

  GtkWidget * selector;
  GtkWidget * sketchpad;

  //--toplevel window
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  sketchbook.window = window;

#ifdef DESKTOP
  gtk_window_set_default_size (GTK_WINDOW (window), 240, 280);
  gtk_window_set_position     (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
#endif
  g_signal_connect (G_OBJECT (window), "delete_event",
                    G_CALLBACK (on_window_sketchpad_destroy), NULL);

  if (gpe_find_icon_pixmap ("this_app_icon", &pixmap, &bitmap)){
    gtk_widget_realize   (window);
    gdk_window_set_icon (window->window, NULL, pixmap, bitmap);
  }

  //--selector
  selector  = selector_gui();
  window_selector_init(window);

  //--sketchpad
  sketchpad = sketchpad_gui(window);
  window_sketchpad_init(window);

  //--packing
  notebook = (GtkNotebook *) gtk_notebook_new();
  sketchbook.notebook = notebook;
  gtk_notebook_set_show_border(notebook, FALSE);
  gtk_notebook_set_show_tabs  (notebook, FALSE);
  g_signal_connect (G_OBJECT (notebook), "switch_page", G_CALLBACK(sketchbook_reset_title), NULL);

  gtk_notebook_insert_page(notebook, selector,  NULL, PAGE_SELECTOR);
  gtk_notebook_insert_page(notebook, sketchpad, NULL, PAGE_SKETCHPAD);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET(notebook));

  //--show up
  //NOTE: could be defined by preference, or command line argument
  gtk_notebook_set_page(notebook, PAGE_SELECTOR);

  gtk_widget_show (GTK_WIDGET(notebook));
  gtk_widget_show (window);

}//gui_init()
