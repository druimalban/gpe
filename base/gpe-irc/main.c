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
#include <stdio.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/render.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>

#include "irc.h"
#include "irc_input.h"
#include "dictionary.h"
#include "networks_config_sql.h"
#include "networks_config.h"

#define WINDOW_NAME "IRC Client"
#define _(_x) gettext (_x)

struct gpe_icon my_icons[] = {
  { "new", "new", NULL },
  { "delete", "delete", NULL },
  { "edit", "edit", NULL },
  { "save", "save", NULL },
  { "properties", "properties", NULL },
  { "preferences", "preferences", NULL },
  { "close", "close", NULL },
  { "stop", "stop", NULL },
  { "error", "error", NULL },
  { "globe", "irc/globe", NULL },
  { "quote", "irc/quote", NULL },
  { "smiley_happy", "irc/smileys/happy", NULL },
  { "icon", PREFIX "/share/pixmaps/gpe-irc.png", NULL },
  {NULL, NULL, NULL}
};

guint window_x = 240, window_y = 310;

GList *servers = NULL;
gboolean users_list_visible = FALSE;

GtkWidget *main_window;
GtkWidget *main_button_hbox;
GtkWidget *main_entry;
GtkWidget *main_text_view;
GtkWidget *main_hpaned;
GtkWidget *users_button_arrow;
GtkWidget *users_button_arrow2;
GtkListStore *users_list_store;
GtkWidget *users_tree_view, *users_scroll;
GtkTextBuffer *text_buffer;
GtkWidget *scroll;

IRCServer *selected_server;
IRCChannel *selected_channel;
GtkWidget *selected_button;

void
toggle_users_list ()
{
  if (users_list_visible == TRUE)
  {
    gtk_widget_hide (users_scroll);
    gtk_arrow_set (GTK_ARROW (users_button_arrow), GTK_ARROW_LEFT, GTK_SHADOW_NONE);
    gtk_arrow_set (GTK_ARROW (users_button_arrow2), GTK_ARROW_LEFT, GTK_SHADOW_NONE);
    users_list_visible = FALSE;
  }
  else
  {
    gtk_widget_show (users_scroll);
    gtk_arrow_set (GTK_ARROW (users_button_arrow), GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
    gtk_arrow_set (GTK_ARROW (users_button_arrow2), GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
    users_list_visible = TRUE;
  }
}

void
kill_widget (GtkWidget *parent, GtkWidget *widget)
{
  gtk_widget_destroy (widget);
}

void
update_window_title ()
{
  gchar *new_title;

  if (selected_server != NULL)
    new_title = g_strdup_printf ("IRC Client - %s @ %s", selected_server->user_info->nick, selected_server->name);
  else
    new_title = g_strdup ("IRC Client");

  gtk_window_set_title (GTK_WINDOW (main_window), new_title);
}

GString *
remove_invalid_utf8_chars (GString *text)
{
  const gchar *invalid_start;

  if (g_utf8_validate (text->str, -1, &invalid_start) == TRUE)
    return text;
  else
    return NULL;
}

gboolean
scroll_text_view_to_bottom ()
{
  GtkTextBuffer *text_buffer;
  GtkTextIter start, end;

  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (main_text_view));
  gtk_text_buffer_get_bounds (text_buffer, &start, &end);
  gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (main_text_view), &end, 0, TRUE, 1.0, 1.0);

  return FALSE;
}

void
update_text_view (GString *text)
{
  GtkTextBuffer *text_buffer;
  GtkTextIter start, end;

  //text = remove_invalid_utf8_chars (text);
  if (text->str != NULL)
  {
    text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (main_text_view));

    gtk_text_buffer_get_bounds (text_buffer, &start, &end);
    gtk_text_buffer_insert (text_buffer, &end, text->str, strlen (text->str));

    gtk_idle_add (scroll_text_view_to_bottom, NULL);
  }
}

void
clear_text_view ()
{
  GtkTextBuffer *text_buffer;
  GtkTextIter start, end;

  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (main_text_view));

  gtk_text_buffer_get_bounds (text_buffer, &start, &end);
  gtk_text_buffer_delete (text_buffer, &start, &end);
}

void
button_clicked (GtkWidget *button)
{
  IRCServer *server;
  IRCChannel *channel;

  if (button != selected_button)
  {
    if (gtk_object_get_data (GTK_OBJECT (button), "type") == IRC_SERVER)
    {
      server = gtk_object_get_data (GTK_OBJECT (button), "IRCServer");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (selected_button), FALSE);
      selected_button = button;
      selected_server = server;
      selected_channel = NULL;
      clear_text_view ();
      update_text_view (server->text);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
    }
    else
    {
      channel = gtk_object_get_data (GTK_OBJECT (button), "IRCChannel");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (selected_button), FALSE);
      selected_button = button;
      selected_server = channel->server;
      selected_channel = channel;
      clear_text_view ();
      update_text_view (channel->text);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
    }
  }
}

void
join_channel (IRCServer *server, gchar *channel_name)
{
  IRCChannel *channel;
  GtkWidget *button;

  if (channel_name[0] == '#' || channel_name[0] == '&')
  {
    //irc_join (server, channel_name);

    channel = g_malloc (sizeof (*channel));
    channel->name = g_strdup (channel_name);
    channel->text = g_string_new ("");
    channel->server = server;
    selected_channel = channel;

    clear_text_view ();

    if (selected_button)
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (selected_button), FALSE);

    button = gtk_toggle_button_new_with_label (channel->name);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
    gtk_box_pack_start (GTK_BOX (main_button_hbox), button, FALSE, FALSE, 0);
    gtk_object_set_data (GTK_OBJECT (button), "type", (gpointer) IRC_CHANNEL);
    gtk_object_set_data (GTK_OBJECT (button), "IRCChannel", (gpointer) channel);
    g_signal_connect (GTK_OBJECT (button), "clicked", G_CALLBACK (button_clicked), NULL);
    gtk_widget_show (button);

    channel->button = button;
    selected_button = button;

    g_hash_table_insert (server->channel, (gpointer) channel->name, (gpointer) channel);
  }
}

void
part_channel (IRCServer *server, IRCChannel *channel)
{
  /* FIXME: It doesn't work that way actually */
  irc_part (server, channel->name, "GPE IRC");

  gtk_widget_destroy (channel->button);
  g_hash_table_remove (server->channel, (gconstpointer) channel->name);
  g_free (channel);
  selected_channel = NULL;
  button_clicked (server->button);
}

void
part_channel_from_hash (gpointer key, gpointer value, gpointer data)
{
  part_channel ((IRCServer *) data, (IRCChannel *) value);
}

void
update_channel_text_from_hash (gpointer key, gpointer value, gpointer data)
{
  ((IRCChannel *) value)->text = g_string_append (((IRCChannel *) value)->text, ((GString *) data)->str);
}

void
disconnect_from_server (IRCServer *server)
{
  irc_quit (server, "GPE IRC");
  server->connected = FALSE;
  g_io_channel_shutdown (server->io_channel, FALSE, NULL);
  g_hash_table_foreach (server->channel, part_channel_from_hash, (gpointer) server);

  servers = g_list_remove (servers, (gconstpointer) server);                                            
  if (servers != NULL)
    button_clicked (((IRCServer *) servers->data)->button);
  else
  {
    clear_text_view ();
    selected_server = NULL;
    selected_channel = NULL;
    selected_button = NULL;
  }

  gtk_widget_destroy (server->button);
  g_free (server);
}

void
close_button_clicked ()
{
  if (selected_button != NULL)
  {
    if (gtk_object_get_data (GTK_OBJECT (selected_button), "type") == IRC_SERVER)
      disconnect_from_server (selected_server);
    else
      part_channel (selected_server, selected_channel);
  }
}

/*
gboolean
watch_disconnect_from_server (GIOChannel *source, GIOCondition condition, gpointer data)
{
  ((IRCServer *) data)->connected = FALSE;
  return FALSE;
}

gboolean
get_data_from_server (GIOChannel *source, GIOCondition condition, gpointer data)
{
  GString *new_text;
  gsize new_string_length;

  new_text = g_string_new ("");

  if (((IRCServer *) data)->connected == FALSE)
  {
    disconnect_from_server ((IRCServer *) data);
    return FALSE;
  }

  if (g_io_channel_read_line_string (source, new_text, NULL, NULL) == G_IO_STATUS_NORMAL)
  {
    ((IRCServer *) data)->text = g_string_append (((IRCServer *) data)->text, new_text->str);
    g_hash_table_foreach (((IRCServer *) data)->channel, update_channel_text_from_hash, (gpointer) new_text);

    if (data == selected_server)
    {
      if (selected_channel == NULL)
        update_text_view (new_text);
      else
	update_text_view (selected_channel->text);
    }
  }

  return TRUE;
}
*/

void
new_connection (GtkWidget *parent, GtkWidget *parent_window)
{
  GtkWidget *server_combo_entry, *nick_entry, *real_name_entry, *password_entry, *button;
  IRCServer *server;

  clear_text_view ();

  server = g_malloc (sizeof (*server));
  server->user_info = g_malloc (sizeof (*server->user_info));
  server->text = g_string_new ("");
  server->channel = g_hash_table_new (g_str_hash, g_str_equal);
  server->prefix = NULL;

  server_combo_entry = gtk_object_get_data (GTK_OBJECT (parent), "server_combo_entry");
  nick_entry = gtk_object_get_data (GTK_OBJECT (parent), "nick_entry");
  real_name_entry = gtk_object_get_data (GTK_OBJECT (parent), "real_name_entry");
  password_entry = gtk_object_get_data (GTK_OBJECT (parent), "password_entry");

  server->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (server_combo_entry)));
  server->user_info->nick = g_strdup (gtk_entry_get_text (GTK_ENTRY (nick_entry)));
  server->user_info->username = g_strdup (gtk_entry_get_text (GTK_ENTRY (nick_entry)));
  server->user_info->real_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (real_name_entry)));
  if (strlen (gtk_entry_get_text (GTK_ENTRY (password_entry))) > 0)
    server->user_info->password = g_strdup (gtk_entry_get_text (GTK_ENTRY (password_entry)));
  else
    server->user_info->password = NULL;

  servers = g_list_append (servers, (gpointer) server);
  selected_server = server;

  if (selected_button)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (selected_button), FALSE);

  button = gtk_toggle_button_new_with_label (server->name);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_box_pack_start (GTK_BOX (main_button_hbox), button, FALSE, FALSE, 0);
  gtk_object_set_data (GTK_OBJECT (button), "type", (gpointer) IRC_SERVER);
  gtk_object_set_data (GTK_OBJECT (button), "IRCServer", (gpointer) server);
  g_signal_connect (GTK_OBJECT (button), "clicked", G_CALLBACK (button_clicked), NULL);
  gtk_widget_show (button);

  server->button = button;
  selected_button = button;

  update_window_title ();

  gtk_widget_destroy (parent_window);

  irc_server_connect (server);
}

void
get_networks (GtkWidget *combo, GHashTable *network_hash)
{
  GList *popdown_strings = NULL;
  GSList *iter;

  iter = sql_networks;
  if (iter != NULL)
  {
    while (iter)
    {
      popdown_strings = g_list_append (popdown_strings, (gpointer) ((struct sql_network *) iter->data)->name);
      if (((struct sql_network *) iter->data)->servers != NULL)
        g_hash_table_insert (network_hash, (gpointer) ((struct sql_network *) iter->data)->name, (gpointer) (struct sql_network *) iter->data);
      iter = iter->next;
    }
    gtk_combo_set_popdown_strings (GTK_COMBO (combo), popdown_strings);
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry), ((struct sql_network *) sql_networks->data)->name);
  }
}

void
select_servers_from_network (GtkWidget *widget, GHashTable *network_hash)
{
  GtkWidget *nick_entry, *real_name_entry, *password_entry;
  GtkWidget *server_combo, *entry;
  gchar *network_name;
  GList *popdown_strings = NULL;
  GSList *iter = NULL;
  
  struct sql_network *network;

  entry = g_object_get_data (G_OBJECT (widget), "entry");
  server_combo = g_object_get_data (G_OBJECT (widget), "server_combo");
  nick_entry = gtk_object_get_data (GTK_OBJECT (widget), "nick_entry");
  real_name_entry = gtk_object_get_data (GTK_OBJECT (widget), "real_name_entry");
  password_entry = gtk_object_get_data (GTK_OBJECT (widget), "password_entry");

  network_name = (gchar *) gtk_entry_get_text (GTK_ENTRY (entry));
  if (strlen (network_name) > 0)
  {
    network = g_hash_table_lookup (network_hash, (gconstpointer) network_name);
    iter = network->servers;
  }
  if (iter != NULL)
  {
    while (iter)
    {
      popdown_strings = g_list_append(popdown_strings, (gpointer) ((struct sql_network_server *) iter->data)->name);
      iter = iter->next;
    }

    gtk_combo_set_popdown_strings (GTK_COMBO (server_combo), popdown_strings);
    g_list_free(popdown_strings);
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (server_combo)->entry), ((struct sql_network_server *) (network->servers->data))->name);
    gtk_entry_set_text (GTK_ENTRY (nick_entry), network->nick);
    gtk_entry_set_text (GTK_ENTRY (real_name_entry), network->real_name);
    gtk_entry_set_text (GTK_ENTRY (password_entry), network->password);
  }
}

void
new_connection_dialog ()
{
  GtkWidget *window, *table, *vbox, *hbox, *button_hbox, *label, *hsep;
  GtkWidget *network_combo, *server_combo, *nick_entry, *real_name_entry, *password_entry;
  GtkWidget *connect_button, *close_button, *network_properties_button;
  GdkPixmap *pmap;
  GdkBitmap *bmap;

  GHashTable *network_hash;
  network_hash = g_hash_table_new (g_str_hash, g_str_equal);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "IRC Client - New Connection");
  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);
  g_signal_connect (G_OBJECT (window), "destroy",
                             G_CALLBACK (kill_widget), window);

  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_set_spacing (GTK_BOX (hbox), 3);
  button_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_set_spacing (GTK_BOX (button_hbox), 6);
  gtk_container_set_border_width (GTK_CONTAINER (button_hbox), 6);

  table = gtk_table_new (2, 5, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);

  label = gtk_label_new ("Network");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Server");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Nickname");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Real Name");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Password");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 4, 5);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  hsep = gtk_hseparator_new ();

  network_combo = gtk_combo_new ();
  server_combo = gtk_combo_new ();

  nick_entry = gtk_entry_new ();
  real_name_entry = gtk_entry_new ();
  password_entry = gtk_entry_new ();

  connect_button = gpe_picture_button (button_hbox->style, "Connect", "globe");
  close_button = gpe_picture_button (button_hbox->style, "Close", "close");
  network_properties_button = gpe_picture_button (button_hbox->style, NULL, "properties");

  g_object_set_data (G_OBJECT (connect_button), "server_combo_entry", (gpointer) GTK_COMBO (server_combo)->entry);
  g_object_set_data (G_OBJECT (connect_button), "nick_entry", (gpointer) nick_entry);
  g_object_set_data (G_OBJECT (connect_button), "real_name_entry", (gpointer) real_name_entry);
  g_object_set_data (G_OBJECT (connect_button), "password_entry", (gpointer) password_entry);
  g_object_set_data (G_OBJECT (network_properties_button), "network_combo", (gpointer) network_combo);
  g_object_set_data (G_OBJECT (network_properties_button), "network_hash", (gpointer) network_hash);
  g_object_set_data (G_OBJECT (GTK_COMBO (network_combo)->entry), "server_combo", (gpointer) server_combo);
  g_object_set_data (G_OBJECT (GTK_COMBO (network_combo)->entry), "entry", (gpointer) GTK_COMBO (network_combo)->entry);
  g_object_set_data (G_OBJECT (GTK_COMBO (network_combo)->entry), "nick_entry", (gpointer) nick_entry);
  g_object_set_data (G_OBJECT (GTK_COMBO (network_combo)->entry), "real_name_entry", (gpointer) real_name_entry);
  g_object_set_data (G_OBJECT (GTK_COMBO (network_combo)->entry), "password_entry", (gpointer) password_entry);

  g_signal_connect (G_OBJECT (connect_button), "clicked",
                             G_CALLBACK (new_connection), window);
  g_signal_connect (GTK_OBJECT (close_button), "clicked",
                             G_CALLBACK (kill_widget), window);
  g_signal_connect (GTK_OBJECT (network_properties_button), "clicked",
                             G_CALLBACK (networks_config_window), NULL);
  g_signal_connect (G_OBJECT (GTK_COMBO(network_combo)->entry), "changed",
                             G_CALLBACK (select_servers_from_network), network_hash);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), button_hbox, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), hsep, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (button_hbox), close_button, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (button_hbox), connect_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), network_combo, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), network_properties_button, FALSE, FALSE, 0);

  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (table), server_combo, 1, 2, 1, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), nick_entry, 1, 2, 2, 3);
  gtk_table_attach_defaults (GTK_TABLE (table), real_name_entry, 1, 2, 3, 4);
  gtk_table_attach_defaults (GTK_TABLE (table), password_entry, 1, 2, 4, 5);

  if (gpe_find_icon_pixmap ("globe", &pmap, &bmap))
    gdk_window_set_icon (window->window, NULL, pmap, bmap);

  get_networks (network_combo, network_hash);

  gtk_widget_show_all (window);
}

gboolean
entry_key_press (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  gchar *entry_text, *display_text;

  //irc_input_entry_key_press (widget, event, data);

  if (selected_channel != NULL)
  {
    entry_text = (gchar *) gtk_entry_get_text (GTK_ENTRY (main_entry));

    if (event->keyval == GDK_Return && strlen (entry_text) > 0)
    {
      irc_privmsg (selected_server, selected_channel->name, entry_text);
      display_text = g_strdup_printf ("%s: %s\n", selected_server->user_info->nick, entry_text);
      selected_server->text = g_string_append (selected_server->text, display_text);
      update_text_view (g_string_new (display_text));
      gtk_entry_set_text (GTK_ENTRY (main_entry), "");
    }
  }

  return FALSE;
}

int
main (int argc, char *argv[])
{
  GtkWidget *vbox, *hbox, *users_button_vbox, *users_button_label, *hsep;
  GtkWidget *users_button, *close_button, *new_connection_button, *quick_button, *smiley_button;
  GdkPixmap *pmap;
  GdkBitmap *bmap;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);
  
  if (networks_sql_start () == -1)
    exit (1);

  setlocale (LC_ALL, "");
  
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (main_window), "IRC Client");
  gtk_widget_set_usize (GTK_WIDGET (main_window), window_x, window_y);
  g_signal_connect (GTK_OBJECT (main_window), "destroy",
		      G_CALLBACK (gtk_exit), NULL);

  gtk_widget_realize (main_window);

  vbox = gtk_vbox_new (FALSE, 0);
  hbox = gtk_hbox_new (FALSE, 0);
  main_hpaned = gtk_hpaned_new ();
  main_button_hbox = gtk_hbox_new (FALSE, 0);
  users_button_vbox = gtk_vbox_new (FALSE, 0);

  gtk_box_set_spacing (GTK_BOX (main_button_hbox), 3);
  gtk_box_set_spacing (GTK_BOX (hbox), 3);

  hsep = gtk_hseparator_new ();

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  users_scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (users_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  main_text_view = gtk_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (main_text_view), GTK_WRAP_WORD);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (main_text_view), FALSE);

  main_entry = gtk_entry_new ();

  users_list_store = gtk_list_store_new (1, G_TYPE_STRING);
  users_tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (users_list_store));

  users_button_arrow = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_NONE);
  users_button_arrow2 = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_NONE);

  close_button = gpe_picture_button (main_button_hbox->style, NULL, "close");
  new_connection_button = gpe_picture_button (hbox->style, NULL, "globe");
  quick_button = gpe_picture_button (hbox->style, NULL, "quote");
  smiley_button = gpe_picture_button (hbox->style, NULL, "smiley_happy");
  users_button = gtk_button_new ();

  g_signal_connect (GTK_OBJECT (close_button), "clicked",
    		      G_CALLBACK (close_button_clicked), NULL);
  g_signal_connect (GTK_OBJECT (new_connection_button), "clicked",
    		      G_CALLBACK (new_connection_dialog), NULL);
  g_signal_connect (GTK_OBJECT (users_button), "clicked",
    		      G_CALLBACK (toggle_users_list), NULL);

  users_button_label = gtk_label_new ("u\ns\ne\nr\ns");
  gtk_misc_set_alignment (GTK_MISC (users_button_label), 0.5, 0.5);

  irc_input_create ("dictionary", main_entry, quick_button, "quick_list", smiley_button, "smiley_list");
  g_signal_connect(GTK_OBJECT (main_entry), "key-press-event",
  	     G_CALLBACK (entry_key_press), NULL);

  gtk_container_add (GTK_CONTAINER (main_window), GTK_WIDGET (vbox));
  gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET (main_text_view));
  gtk_container_add (GTK_CONTAINER (users_scroll), GTK_WIDGET (users_tree_view));
  gtk_container_add (GTK_CONTAINER (users_button), GTK_WIDGET (users_button_vbox));
  gtk_box_pack_start (GTK_BOX (users_button_vbox), users_button_arrow, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (users_button_vbox), users_button_label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (users_button_vbox), users_button_arrow2, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), main_button_hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hsep, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), main_hpaned, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (main_button_hbox), close_button, FALSE, FALSE, 0);
  gtk_paned_pack1 (GTK_PANED (main_hpaned), scroll, FALSE, TRUE);
  gtk_paned_pack2 (GTK_PANED (main_hpaned), users_scroll, FALSE, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), quick_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), smiley_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), main_entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), new_connection_button, FALSE, FALSE, 0);

  if (gpe_find_icon_pixmap ("icon", &pmap, &bmap))
    gdk_window_set_icon (main_window->window, NULL, pmap, bmap);

  gtk_widget_show_all (main_window);
  gtk_widget_hide (users_scroll);

  gtk_widget_grab_focus (main_entry);

  gtk_main ();

  return 0;
}
