/*
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2005 Philippe De Swert <philippedeswert@scarlet.be>
 * 
 * Transition from gtkCtree to GtkTreeView
 * updating program structure
 * new interface for the journal
 * Copyright (C) 2006 by Michele Giorgini <md6604@mclink.it> 
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
#include <string.h>

// hildon includes
#ifdef IS_HILDON
#if HILDON_VER > 0
#include <hildon/hildon-program.h>
#include <hildon/hildon-window.h>
#else
#include <hildon-widgets/hildon-app.h>
#include <hildon-widgets/hildon-appview.h>
#endif /* HILDON_VER */
#include <libosso.h>
#define OSSO_SERVICE_NAME "gpe_timesheet"
#endif /* IS_HILDON */

// gdk-gtk-gpe includes
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/smallbox.h>
#include <gpe/errorbox.h>
#include <gpe/gpehelp.h>
#include <gpe/gpedialog.h>

// timesheet includes
#include "sql.h"
#include "journal.h"
#include "ui.h"
#include "main.h"

#define _(_x) gettext(_x)
#define JOURNAL_FILE "/tmp/journal.html"

static struct gpe_icon my_icons[] = {
// for hildon compiling and testing remember to set main.h PREFIXES
// and to comment out gpe-timesheet line
// 'til hildonization is completed
  { "clock", MY_PIXMAPS_DIR "/clock.png"},
  { "stop_clock", MY_PIXMAPS_DIR "/stop_clock.png" },
  { "tick", MY_PIXMAPS_DIR "/tick.png"},
  { "gpe-timesheet", PREFIX "/share/pixmaps/gpe-timesheet.png" },
  { "edit", MY_PIXMAPS_DIR "/edit.png"},
  { "journal", MY_PIXMAPS_DIR "/list-view.png"},
  { "media_play", MY_PIXMAPS_DIR "/media-play.png"},
  { "media_stop", MY_PIXMAPS_DIR "/media-stop.png"},
  { NULL, NULL }

};

int video;
gboolean large_screen;
gboolean mode_landscape;
GtkWidget *main_window;

// hildon main appview definition and specific functions
#ifdef IS_HILDON
static void osso_top_callback (const gchar* arguments, gpointer ptr)
{
    GtkWindow *window = ptr;

    gtk_window_present (GTK_WINDOW (window));
}

#if HILDON_VER > 0
static gboolean key_press_cb(GtkWidget *w, GdkEventKey *event, GtkWindow *window)
{
#ifndef HILDON_FULLSCREEN_KEY
#define HILDON_FULLSCREEN_KEY GDK_F6
#endif
  static gboolean fullscreen = FALSE;

  switch (event->keyval)
  {
    case HILDON_FULLSCREEN_KEY: {
      if (fullscreen) {
	gtk_window_unfullscreen(window);
	fullscreen = FALSE;
      } else {
 	gtk_window_fullscreen(window);
	fullscreen = TRUE;
      }
   }
  }

  return FALSE;
}
#else
GtkWidget *main_appview;

static gboolean key_press_cb(GtkWidget *w, GdkEventKey *event, HildonApp *app)
{
  HildonAppView *appview = hildon_app_get_appview(app);

  switch (event->keyval)
  {
    case HILDON_FULLSCREEN_KEY: {
      if (hildon_appview_get_fullscreen(appview))
        hildon_appview_set_fullscreen(appview, FALSE);
      else
        hildon_appview_set_fullscreen(appview, TRUE);
    }
  }

  return FALSE;

}
#endif /* HILDON_VER */
#endif /* IS_HILDON */


static void
open_window (void)
{
  GtkWidget *main_vbox;

  large_screen = (gdk_screen_width() > 400);
  mode_landscape = (gdk_screen_width() > gdk_screen_height());

#ifdef IS_HILDON
// a lot of hildon stuff for hildon integration
  GtkToolbar *main_toolbar;
  osso_context_t *context;

#if HILDON_VER > 0
  HildonProgram *program = HILDON_PROGRAM (hildon_program_get_instance());
  g_set_application_name (_("TimeTracker"));
  main_window = GTK_WIDGET (hildon_window_new());
  hildon_program_add_window (program, HILDON_WINDOW(main_window));
  main_vbox = GTK_WIDGET(create_interface(main_window));
  gtk_container_add (GTK_CONTAINER(main_window), main_vbox);

  /* this code is not to be killed by osso */
  context = osso_initialize (OSSO_SERVICE_NAME, VERSION, TRUE, NULL);
  g_assert(context);
  osso_application_set_top_cb(context, osso_top_callback, ( gpointer )main_window);

#else
  GtkWidget *app = hildon_app_new();
  hildon_app_set_two_part_title(HILDON_APP(app), TRUE);
  hildon_app_set_title (HILDON_APP(app), _("TimeTracker"));
  main_appview = hildon_appview_new (_("List"));
  hildon_app_set_appview (HILDON_APP(app), HILDON_APPVIEW(main_appview));
  main_window = main_appview;

  main_vbox = GTK_WIDGET(create_interface(main_window));

  gtk_container_add (GTK_CONTAINER(main_window), main_vbox);

  // this one is hildon specific to listen hardware buttons
  g_signal_connect(G_OBJECT(app),"key_press_event", G_CALLBACK(key_press_cb), app);

  /* this code is not to be killed by osso */
  context = osso_initialize (OSSO_SERVICE_NAME, VERSION, TRUE, NULL);
  g_assert(context);
  osso_application_set_top_cb(context, osso_top_callback, ( gpointer )app);

  gtk_widget_show (app);
  gtk_widget_show (main_appview);
#endif /* HILDON_VER */

#else

  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (main_window), 780, 400);
  gtk_window_set_title (GTK_WINDOW (main_window), _("TimeTracker"));
  gpe_set_window_icon (main_window, "gpe-timesheet");
  main_vbox = GTK_WIDGET(create_interface(main_window));
  gtk_container_add (GTK_CONTAINER(main_window), main_vbox);
#endif /* IS_HILDON */

  g_signal_connect (G_OBJECT(main_window), "delete-event", G_CALLBACK (gtk_main_quit), NULL);

  gtk_widget_show (main_window);
}

int
main(int argc, char *argv[])
{
  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  if (sql_start () == FALSE)
    exit (1);

  if (sql_append_todo () == FALSE)
    exit (1);

  open_window ();

  gtk_main();

  return 0;
}
