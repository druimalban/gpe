/*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib.h>
#include <locale.h>
#include <libintl.h>
#include <time.h>

#include <gtk/gtk.h>

#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/render.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>

#include "irc.h"

#define WINDOW_NAME "IRC Client"
#define _(_x) gettext (_x)

struct gpe_icon my_icons[] = {
  { "new", "new" },
  { "delete", "delete" },
  { "edit", "edit" },
  { "properties", "properties" },
  { "cancel", "cancel" },
  { "stop", "stop" },
  { "error", "error" },
  { "globe", "irc/globe" },
  { "icon", PREFIX "/share/pixmaps/gpe-irc.png" },
  {NULL, NULL}
};

guint window_x = 240, window_y = 310;

GHashTable *servers;

GtkWidget *main_window;
GtkWidget *main_button_hbox;
GtkWidget *main_entry;
GtkWidget *main_text_view;
GtkWidget *main_hbox;
GtkWidget *nick_label;
GtkTextBuffer *text_buffer;

void
bind_server_feedback (IRCServer *server)
{
  GtkTextBuffer *text_buffer;
  GtkTextIter start, end;
  gchar *text;

  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (main_text_view));

  while (1)
  {
    //printf ("Updating text...\n");
    text = irc_server_read (server);
    gtk_text_buffer_get_bounds (text_buffer, &start, &end);
    gtk_text_buffer_insert (text_buffer, &end, text, strlen (text));
  }
}

void
connection_init (IRCServer *server)
{
  gtk_label_set_text (GTK_LABEL (nick_label), server->user_info->nick);
  bind_server_feedback (server);
}

int
main (int argc, char *argv[])
{
  GtkWidget *vbox, *hbox, *scroll, *hsep;
  GtkWidget *users_button, *close_button, *new_connection_button;
  IRCServer *server;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");
  
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  servers = g_hash_table_new (g_str_hash, g_str_equal);
  server = g_malloc (sizeof (*server));
  server->user_info = g_malloc (sizeof (*server->user_info));

  server->name = g_strdup ("irc.jecx.net");
  server->user_info->nick = g_strdup ("dc_1337-irc");
  server->user_info->username = g_strdup ("dc_1337-irc");
  server->user_info->real_name = g_strdup ("dc_1337-irc");
  server->user_info->password = NULL;
  irc_server_connect (server);

  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (main_window), "IRC Client");
  gtk_widget_set_usize (GTK_WIDGET (main_window), window_x, window_y);
  gtk_signal_connect (GTK_OBJECT (main_window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  gtk_widget_realize (main_window);

  vbox = gtk_vbox_new (FALSE, 0);
  hbox = gtk_hbox_new (FALSE, 0);
  main_hbox = gtk_hbox_new (FALSE, 0);
  main_button_hbox = gtk_hbox_new (FALSE, 0);

  hsep = gtk_hseparator_new ();

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  main_text_view = gtk_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (main_text_view), GTK_WRAP_WORD);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (main_text_view), FALSE);

  main_entry = gtk_entry_new ();

  close_button = gpe_picture_button (main_button_hbox->style, NULL, "cancel");
  new_connection_button = gpe_picture_button (hbox->style, NULL, "globe");
  users_button = gtk_button_new_with_label ("u\ns\ne\nr\ns");

  nick_label = gtk_label_new ("\t");

  gtk_container_add (GTK_CONTAINER (main_window), GTK_WIDGET (vbox));
  gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET (main_text_view));
  gtk_box_pack_start (GTK_BOX (vbox), main_button_hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hsep, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), main_hbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (main_button_hbox), close_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_hbox), scroll, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (main_hbox), users_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), nick_label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), main_entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), new_connection_button, FALSE, FALSE, 0);

  gtk_widget_show_all (main_window);

  gtk_widget_grab_focus (main_entry);

  connection_init (server);

  return 0;
}
