/*
 * gpe-synctool
 *
 * Synchronisation tool for gpesyncd.
 * 
 * (c) 2006, 2007 Florian Boor  <florian@linuxtogo.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * Dock applet and application control.
 */
#include <stdio.h>
#include <stdlib.h>
#include <libintl.h>
#include <locale.h>
#include <unistd.h>
#include <signal.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/tray.h>
#include <gpe/infoprint.h>
#include <gpe/popup.h>
#include <gpe/spacing.h>

#define _(x) gettext(x)

#define DAEMON_DEFAULT_PORT  6446
#define COMMAND_DAEMON_START "gpesyncd"
#define COMMAND_APP          "gpe-conf"
#define COMMAND_CONFIG       COMMAND_APP " sync"  


struct gpe_icon my_icons[] = {
  {"gpe-synctool", "sync-on"},
  {"sync-on",      "sync-on"},
  {"sync-off",     "sync-off"},
  {NULL}
};

typedef enum
{
  NUM_ICONS
}
n_images;


static GtkWidget *window;
static GtkWidget *menu;
static GtkWidget *icon;

static sync_active = FALSE;
static GPid daemon_pid = -1;

static void
update_icon (gint size)
{
  GdkBitmap *bitmap;
  GdkPixbuf *sbuf, *dbuf;

  if (size <= 0)
    size = gdk_pixbuf_get_width (gtk_image_get_pixbuf (GTK_IMAGE (icon)));

  sbuf = gpe_find_icon (sync_active ? "sync-on" : "sync-off");
  dbuf = gdk_pixbuf_scale_simple (sbuf, size, size, GDK_INTERP_HYPER);
  gdk_pixbuf_render_pixmap_and_mask (dbuf, NULL, &bitmap, 64);
  gtk_widget_shape_combine_mask (GTK_WIDGET (window), NULL, 1, 0);
  gtk_widget_shape_combine_mask (GTK_WIDGET (window), bitmap, 1, 0);
  gdk_bitmap_unref (bitmap);
  gtk_image_set_from_pixbuf (GTK_IMAGE (icon), dbuf);
}


static gboolean
cancel_message (gpointer data)
{
  guint id = (guint) data;
  gpe_system_tray_cancel_message (window->window, id);
  return FALSE;
}

static void
sync_daemon_stop (void)
{
  if (daemon_pid > 0)
    kill (daemon_pid, SIGTERM);
}

static void
sync_daemon_start (void)
{
  gchar *params;
  gchar *argv[] = { COMMAND_DAEMON_START, params, NULL };
	
  params = g_strdup_printf ("-D %i", DAEMON_DEFAULT_PORT);
  g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH,
                 NULL, NULL, &daemon_pid, NULL);
  g_free (params);
}


static void
do_sync_toggle (GtkCheckMenuItem *item, gpointer userdata)
{
  if (gtk_check_menu_item_get_active (item))
    {
      sync_daemon_start ();
      sync_active = TRUE;
    }
  else
    {
      sync_daemon_stop ();
      sync_active = FALSE;
    }
  update_icon (-1);
}

static void
do_sync_config (void)
{
  GError *err = NULL;

  if (!g_spawn_command_line_async (COMMAND_CONFIG, &err))
    {
      gpe_error_box (_("Could not start configuration tool!"));
      g_printerr ("Err gpe-synctool: %s\n", err->message);
      g_error_free (err);
    }
}

static void
app_shutdown (void)
{
  if (sync_active) 
    sync_daemon_stop();
    
  gtk_main_quit ();
}

static void
sigterm_handler (int sig)
{
  app_shutdown ();
}


static void
clicked (GtkWidget * w, GdkEventButton * ev, gpointer data)
{
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, gpe_popup_menu_position,
		  w, ev->button, ev->time);
}


/* handle resizing */
gboolean
external_event (GtkWindow * window, GdkEventConfigure * event,
		gpointer user_data)
{
  gint size;

  if (event->type == GDK_CONFIGURE)
    {
      size = (event->width < event->height) ? event->height : event->width;
      update_icon (size);
    }
  return FALSE;
}


int
main (int argc, char *argv[])
{
  Display *dpy;
  GtkWidget *menu_toggle;
  GtkWidget *menu_remove;
  GtkWidget *menu_config;
  GtkTooltips *tooltips;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

  window = gtk_plug_new (0);
  gtk_window_set_resizable (GTK_WINDOW (window), TRUE);
  gtk_widget_realize (window);

  gtk_window_set_title (GTK_WINDOW (window), _("Sync Control"));

  signal (SIGTERM, sigterm_handler);

  menu = gtk_menu_new ();
  menu_toggle = gtk_check_menu_item_new_with_label (_("Enable sync access"));
  menu_config = gtk_menu_item_new_with_label (_("Configure Syncing"));
  menu_remove = gtk_menu_item_new_with_label (_("Remove from dock"));

  g_signal_connect (G_OBJECT (menu_toggle), "activate",
		    G_CALLBACK (do_sync_toggle), NULL);
  g_signal_connect (G_OBJECT (menu_config), "activate",
		    G_CALLBACK (do_sync_config), NULL);
  g_signal_connect (G_OBJECT (menu_remove), "activate",
		    G_CALLBACK (app_shutdown), NULL);

  gtk_menu_append (GTK_MENU (menu), menu_toggle);
  gtk_menu_append (GTK_MENU (menu), menu_config);
  gtk_menu_append (GTK_MENU (menu), menu_remove);

  gtk_widget_show_all (menu);
  
  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  icon = gtk_image_new_from_pixbuf (gpe_find_icon (sync_active ? "sync-on" :
						   "sync-off"));
  gtk_misc_set_alignment (GTK_MISC (icon), 0, 0);
  gtk_widget_show (icon);
  gpe_set_window_icon (window, "gpe-synctool");

  tooltips = gtk_tooltips_new ();
  gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), window,
			_("This is gpe-synctool - the syncing control applet."), NULL);

  g_signal_connect (G_OBJECT (window), "button-press-event",
		    G_CALLBACK (clicked), NULL);
  g_signal_connect (G_OBJECT (window), "configure-event",
		    G_CALLBACK (external_event), NULL);
  gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK);

  gtk_container_add (GTK_CONTAINER (window), icon);

  dpy = GDK_WINDOW_XDISPLAY (window->window);

  gtk_widget_show (window);

  gpe_system_tray_dock (window->window);

  gtk_main ();

  exit (0);
}
