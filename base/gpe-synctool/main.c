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

#define INFO_TEXT _("Information about setting up PIM data synchronisation " \
                  "for GPE \n\n" \
                  "gpe-synctools is intended to be used for PIM data syncronisation " \
                  "in trusted networks. It provides sync access to your device "\
                  "without the need of setting up a SSH connection. To set up syncronising " \
                  "just follow these steps:\n" \
                  "1. Set up a network connection between your PC and your device running GPE."\
                 "  or example you can use WiFi or USB for this purpose.\n"\
                  "2. Create a file 'gpesyncd.allow' in the '.gpe' directory "\
                  " in your home directory on your device."\
                  " The file needs to contain the IP address of your PC (or "\
                  " one each line if you intend to use multiple PCs).\n"\
                  "3. Configure the synchronisation software of your PC to connect to  1234 "\
                  " port 6446 of your device directly (not using ssh).\n"\
                  "4. Turn on sync access using this tool and start the sync process"\
                  " on your PC.\n" \
                  "In untrusted networks it is highly recommended to use ssh for "\
                  "sync access to your device. Please refer to the OpenSync GPE "\
                  "plugin documentation for setting up this.")

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

  params = g_strdup_printf ("%i", DAEMON_DEFAULT_PORT);
  gchar *argv[]  = { COMMAND_DAEMON_START, "-D", params, NULL };
	
  if (!g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH,
                 NULL, NULL, &daemon_pid, NULL))
    gpe_error_box (_("Unable to start gpesyncd."));
  
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
do_sync_info (void)
{
  GtkWidget *dialog, *text, *sw;
  GtkTextBuffer *buf;
    
  dialog = gtk_dialog_new_with_buttons (_("Sync Information"), 
                                        GTK_WINDOW (window), 
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
  gtk_window_set_default_size (GTK_WINDOW (dialog), 220, 320);
    
  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), 
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  
  text = gtk_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text), GTK_WRAP_WORD);
  buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
  gtk_text_buffer_set_text (buf, INFO_TEXT, -1);
  gtk_container_add (GTK_CONTAINER (sw), text);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), sw, 
                      TRUE, TRUE, 0); 
  g_signal_connect_swapped (dialog, "response", 
                             G_CALLBACK (gtk_widget_destroy),
                             dialog);
  gtk_widget_show_all (dialog);
  gtk_dialog_run (GTK_DIALOG(dialog));
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
      size = (event->width > event->height) ? event->height : event->width;
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
  GtkWidget *menu_config, *menu_info;
  GtkWidget *menu_sep1, *menu_sep2;
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
  menu_info   = gtk_menu_item_new_with_label (_("Setup information"));
  menu_remove = gtk_menu_item_new_with_label (_("Remove from dock"));
  menu_sep1 = gtk_separator_menu_item_new ();
  menu_sep2 = gtk_separator_menu_item_new ();

  g_signal_connect (G_OBJECT (menu_toggle), "activate",
		    G_CALLBACK (do_sync_toggle), NULL);
  g_signal_connect (G_OBJECT (menu_config), "activate",
		    G_CALLBACK (do_sync_config), NULL);
  g_signal_connect (G_OBJECT (menu_info), "activate",
		    G_CALLBACK (do_sync_info), NULL);
  g_signal_connect (G_OBJECT (menu_remove), "activate",
		    G_CALLBACK (app_shutdown), NULL);

  gtk_menu_append (GTK_MENU (menu), menu_toggle);
  gtk_menu_append (GTK_MENU (menu), menu_sep1);
  gtk_menu_append (GTK_MENU (menu), menu_info);
  gtk_menu_append (GTK_MENU (menu), menu_config);
  gtk_menu_append (GTK_MENU (menu), menu_sep2);
  gtk_menu_append (GTK_MENU (menu), menu_remove);

  /*fixme: not yet available */
  gtk_widget_set_sensitive (menu_config, FALSE);

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
