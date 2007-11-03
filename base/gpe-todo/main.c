/*
 * Copyright (C) 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2005, 2006 Florian Boor <florian@kernelconcepts.de>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>
#include <locale.h>

/* Hildon includes */
#ifdef IS_HILDON
#if HILDON_VER == 1
#include <hildon/hildon-program.h>
#include <hildon/hildon-window.h>
#else
#include <hildon-widgets/hildon-program.h>
#include <hildon-widgets/hildon-window.h>
#endif

#include <libosso.h>

#define OSSO_SERVICE_NAME "gpe_todo"
#endif

/* GTK and GPE includes */
#include <gtk/gtkmain.h>
#include <gpe/pixmaps.h>
#include <gpe/init.h>
#include <gpe/pim-categories.h>

#include "todo.h"

#define MY_PIXMAPS_DIR PREFIX "/share/gpe-todo"
#define MY_PREFIX PREFIX 

static struct gpe_icon my_icons[] = {
  { "high", MY_PIXMAPS_DIR "/flag-16.png" },
  { "icon", MY_PREFIX "/share/pixmaps/gpe-todo.png" },
  { "tick-box", MY_PIXMAPS_DIR "/tick-box.png" },
  { "notick-box", MY_PIXMAPS_DIR "/notick-box.png" },
  { "bar-box", MY_PIXMAPS_DIR "/bar-box.png" },
  { "dot-box", MY_PIXMAPS_DIR "/dot-box.png" },
  { NULL, NULL }
};

#define _(_x) gettext(_x)

GtkWidget *the_notebook;
GtkWidget *window;
gboolean large_screen;
gboolean mode_landscape;

extern GtkWidget *top_level (GtkWidget *window);

#ifdef IS_HILDON

static void 
osso_top_callback (const gchar* arguments, gpointer ptr)
{
    GtkWindow *window = ptr;

    gtk_window_present (GTK_WINDOW (window));
}

#endif

static void
open_window (void)
{
  GtkWidget *top;
#ifdef IS_HILDON
  HildonProgram *app;
  osso_context_t *context;
#endif
    
  large_screen = (gdk_screen_width() > 400);
  mode_landscape = (gdk_screen_width() > gdk_screen_height());
    
#ifdef IS_HILDON
  app = HILDON_PROGRAM ( hildon_program_get_instance () );
  g_set_application_name ( _("ToDo") );
  window = hildon_window_new();
  gtk_window_set_title (GTK_WINDOW (window), _("List"));
  hildon_program_add_window (app, HILDON_WINDOW (window));
  gtk_widget_show_all (window);
	
  /* tell osso we are here or it will kill us */
  context = osso_initialize (OSSO_SERVICE_NAME, VERSION, TRUE, NULL);
  g_assert(context);
  osso_application_set_top_cb(context, osso_top_callback, ( gpointer )app);
#else
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), 320, 400);
  gtk_window_set_title (GTK_WINDOW (window), _("To-do list"));
  gpe_set_window_icon (window, "icon");
#endif

  top = top_level (window);
  gtk_container_add (GTK_CONTAINER (window), top);

  g_signal_connect (G_OBJECT (window), "delete-event",
		    G_CALLBACK (gtk_main_quit), NULL);

  gtk_widget_show_all (window);
}

int
main (int argc, char *argv[])
{
  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");
  
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  if (todo_db_start ())
    exit (1);

  if (gpe_pim_categories_init () == FALSE)
    exit (1);

  open_window ();
  
  gtk_main ();

  return 0;
}
