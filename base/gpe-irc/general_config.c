/*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib.h>

#include <gtk/gtk.h>

#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/render.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>

#include "general_config.h"
#include "general_config_sql.h"

static GtkWidget *
make_option_menu (gchar *tag_name)
{
  GtkWidget *menu, *menu_item, *option_menu, *swatch;

  menu = gtk_menu_new ();
  option_menu = gtk_option_menu_new ();

  menu_item = gtk_image_menu_item_new_with_label ("Black");
  swatch = gpe_render_icon (menu_item->style, gpe_find_icon ("black"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), swatch);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show (swatch);
  gtk_widget_show (menu_item);

  menu_item = gtk_image_menu_item_new_with_label ("White");
  swatch = gpe_render_icon (menu_item->style, gpe_find_icon ("white"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), swatch);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show (swatch);
  gtk_widget_show (menu_item);

  menu_item = gtk_image_menu_item_new_with_label ("Red");
  swatch = gpe_render_icon (menu_item->style, gpe_find_icon ("red"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), swatch);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show (swatch);
  gtk_widget_show (menu_item);

  menu_item = gtk_image_menu_item_new_with_label ("Blue");
  swatch = gpe_render_icon (menu_item->style, gpe_find_icon ("blue"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), swatch);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show (swatch);
  gtk_widget_show (menu_item);

  menu_item = gtk_image_menu_item_new_with_label ("Green");
  swatch = gpe_render_icon (menu_item->style, gpe_find_icon ("green"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), swatch);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show (swatch);
  gtk_widget_show (menu_item);

  menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_BOLD, NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show (menu_item);

  menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_ITALIC, NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show (menu_item);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);

  return option_menu;
}

static void
window_close (GtkWidget *window)
{
  gtk_widget_destroy (window);
}

static void
window_close_and_save (GtkWidget *window)
{
  gtk_widget_destroy (window);
}

void
general_config_window ()
{
  GtkWidget *window, *vbox, *notebook, *notebook_label, *scroll, *table, *button_hbox;
  GtkWidget *save_button, *close_button;
  GtkWidget *label, *text_option_menu, *bg_option_menu, *nick_option_menu, *action_option_menu, *own_nick_option_menu, *channel_option_menu, *nick_ops_option_menu, *nick_highlight_option_menu;
  GdkPixmap *pmap;
  GdkBitmap *bmap;
  
  guint window_x = 240, window_y = 310;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "IRC Client - Preferences");
  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      G_CALLBACK (window_close), window);

  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_box_set_spacing (GTK_BOX (vbox), 6);
  button_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_set_spacing (GTK_BOX (button_hbox), 6);
  gtk_box_set_spacing (GTK_BOX (button_hbox), 6);

  notebook = gtk_notebook_new ();

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  table = gtk_table_new (8, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroll), table);

  label = gtk_label_new ("Normal text");
  text_option_menu = make_option_menu ("tag_text");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (table), text_option_menu, 1, 2, 0, 1);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Background");
  bg_option_menu = make_option_menu ("tag_bg");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), bg_option_menu, 1, 2, 1, 2);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Action astrix");
  action_option_menu = make_option_menu ("tag_action");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
  gtk_table_attach_defaults (GTK_TABLE (table), action_option_menu, 1, 2, 2, 3);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Channel names");
  channel_option_menu = make_option_menu ("tag_channel");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);
  gtk_table_attach_defaults (GTK_TABLE (table), channel_option_menu, 1, 2, 3, 4);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Nicknames");
  nick_option_menu = make_option_menu ("tag_nick");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 4, 5);
  gtk_table_attach_defaults (GTK_TABLE (table), nick_option_menu, 1, 2, 4, 5);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Own nick");
  own_nick_option_menu = make_option_menu ("tag_own_nick");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 5, 6);
  gtk_table_attach_defaults (GTK_TABLE (table), own_nick_option_menu, 1, 2, 5, 6);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Nick operations");
  nick_ops_option_menu = make_option_menu ("tag_nick_ops");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 6, 7);
  gtk_table_attach_defaults (GTK_TABLE (table), nick_ops_option_menu, 1, 2, 6, 7);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Nick highlight");
  nick_highlight_option_menu = make_option_menu ("tag_nick_highlight");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 7, 8);
  gtk_table_attach_defaults (GTK_TABLE (table), nick_highlight_option_menu, 1, 2, 7, 8);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  notebook_label = gtk_label_new ("Colours");
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scroll, notebook_label);

  close_button = gpe_button_new_from_stock (GTK_STOCK_CLOSE, GPE_BUTTON_TYPE_BOTH);
  save_button = gpe_button_new_from_stock (GTK_STOCK_SAVE, GPE_BUTTON_TYPE_BOTH);

  g_signal_connect (G_OBJECT (close_button), "clicked",
                             G_CALLBACK (window_close), window);
  g_signal_connect (G_OBJECT (save_button), "clicked",
                             G_CALLBACK (window_close_and_save), window);

  g_object_set_data (G_OBJECT (save_button), "text_option_menu", (gpointer) text_option_menu);
  g_object_set_data (G_OBJECT (save_button), "bg_option_menu", (gpointer) bg_option_menu);
  g_object_set_data (G_OBJECT (save_button), "action_option_menu", (gpointer) action_option_menu);
  g_object_set_data (G_OBJECT (save_button), "channel_option_menu", (gpointer) channel_option_menu);
  g_object_set_data (G_OBJECT (save_button), "nick_option_menu", (gpointer) nick_option_menu);
  g_object_set_data (G_OBJECT (save_button), "own_nick_option_menu", (gpointer) own_nick_option_menu);
  g_object_set_data (G_OBJECT (save_button), "nick_ops_option_menu", (gpointer) nick_ops_option_menu);
  g_object_set_data (G_OBJECT (save_button), "nick_highlight_option_menu", (gpointer) nick_ops_option_menu);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_hbox, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (button_hbox), close_button, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (button_hbox), save_button, FALSE, FALSE, 0);

  if (gpe_find_icon_pixmap ("properties", &pmap, &bmap))
    gdk_window_set_icon (window->window, NULL, pmap, bmap);

  gtk_widget_show_all (window);
}

