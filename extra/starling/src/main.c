/*
 * Copyright (C) 2006 Alberto García Hierro
 *      <skyhusker@handhelds.org>
 * Copyright (C) 2007 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <string.h>
#include <locale.h>
#include <gst/gst.h>

#include "starling.h"
#include "callbacks.h"
#include "config.h"
#include "lastfm.h"
#include "errorbox.h"
#include "lyrics.h"

#include <gtk/gtk.h>

#ifdef ENABLE_GPE
#   include <gpe/init.h>
#endif

#ifdef IS_HILDON
/* Hildon includes */
#include <hildon-widgets/hildon-app.h>
#include <hildon-widgets/hildon-appview.h>
#include <libosso.h>
#define APPLICATION_DBUS_SERVICE "starling"
#endif

#define BUTTONSIZE 32

static void
create_button (GtkWidget **button, const gchar *stock, GtkWidget *box)
{
    GtkWidget *pix;

    *button = gtk_button_new ();
    pix = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_SMALL_TOOLBAR);

    gtk_container_add (GTK_CONTAINER (*button), pix);

    gtk_widget_set_size_request (*button, -1, BUTTONSIZE);

    gtk_box_pack_start (GTK_BOX (box), *button, TRUE, TRUE, 1);
}

static void
title_data_func (GtkCellLayout *cell_layout,
		 GtkCellRenderer *cell_renderer,
		 GtkTreeModel *model,
		 GtkTreeIter *iter,
		 gpointer data)
{
  char *artist;
  char *title;
  char *buffer = NULL;
  gtk_tree_model_get (model, iter, PL_COL_TITLE, &buffer,
		      PL_COL_ARTIST, &artist, -1);
  if (buffer)
    title = buffer;
  else
    /* Try to show the last three path components as the directory
       structure is often artist/album/file.  */
    {
      gtk_tree_model_get (model, iter, PL_COL_SOURCE, &buffer, -1);
      if (buffer)
	{
	  char *slashes[3];
	  int i = 0;
	  char *t = strchr (buffer, '/');
	  do
	    {
	      while (t[1] == '/')
		t ++;
	      slashes[(i ++) % 3] = t;
	    }
	  while ((t = strchr (t + 1, '/')));

	  if (i >= 3)
	    title = slashes[(i - 3) % 3] + 1;
	  else if (i)
	    title = slashes[0] + 1;
	  else
	    title = buffer;
	}
      else
	title = "unknown";
    }

  if (artist)
    {
      char *t = g_strdup_printf ("%s - %s", artist, title);
      g_free (buffer);
      title = buffer = t;
    }

  g_object_set (cell_renderer, "text", title, NULL);
  g_free (buffer);

  int uid;
  gtk_tree_model_get (model, iter, PL_COL_INDEX, &uid, -1);
  g_object_set (cell_renderer,
		"cell-background-set",
		uid == play_list_get_current (PLAY_LIST (model)),
		NULL);
}

int
main(int argc, char *argv[])
{
  Starling st_;
  Starling *st = &st_;

  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
    
#ifdef ENABLE_GPE
  gpe_application_init (&argc, &argv);
#else
  gtk_init (&argc, &argv);
#endif

#ifdef IS_HILDON
  osso_context_t *osso_context;

  /* Initialize maemo application */
  osso_context = osso_initialize(APPLICATION_DBUS_SERVICE, "0.1", TRUE, NULL);

  /* Check that initialization was ok */
  if (osso_context == NULL)
    return OSSO_ERROR;
#endif

  const char *home = g_get_home_dir ();
  char *file = g_strdup_printf ("%s/.gpe/playlist", home);

  GError *err = NULL;
  st->pl = play_list_open (file, &err);
  g_free (file);
  if (err)
    {
      g_warning (err->message);
      starling_error_box (err->message);
      g_error_free (err);
      exit (1);
    }

  /* Build the GUI.  */
  GtkWidget *l_button_box = NULL;
  GtkWidget *hbox1 = NULL;
  GtkWidget *hbox2 = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *separator = NULL;
  GtkWidget *label = NULL;
  GtkCellRenderer *renderer = NULL;
  GtkWidget *scrolled = NULL;
    
#ifdef IS_HILDON
  st->window = hildon_app_new ();
  hildon_app_set_two_part_title (HILDON_APP (st->window), FALSE);
  GtkWidget *main_appview = hildon_appview_new (_("Main"));
  hildon_app_set_appview (HILDON_APP (st->window),
			  HILDON_APPVIEW (main_appview));
#else    
  st->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#endif

  gtk_window_set_title (GTK_WINDOW (st->window), "Starling");
    
  GtkBox *main_box = GTK_BOX (gtk_vbox_new (FALSE, 5));
#if IS_HILDON
  gtk_container_add (GTK_CONTAINER (main_appview), GTK_WIDGET (main_box));
#else
  gtk_container_add (GTK_CONTAINER (st->window), GTK_WIDGET (main_box));
#endif

  /* The toolbar.  */
  GtkWidget *toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			       GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  GTK_WIDGET_UNSET_FLAGS (toolbar, GTK_CAN_FOCUS);
  gtk_widget_show (toolbar);

#ifdef IS_HILDON
  hildon_appview_set_toolbar (HILDON_APPVIEW (main_appview),
			      GTK_TOOLBAR (toolbar));
  gtk_widget_show_all (main_appview);
#else
  gtk_box_pack_start (GTK_BOX (main_box), toolbar, FALSE, FALSE, 0);
#endif

  /* Previous button.  */
  GtkToolItem *item;
  item = gtk_tool_button_new_from_stock (GTK_STOCK_MEDIA_PREVIOUS);
  st->prev = GTK_WIDGET (item);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  /* Play/pause.  */
  item = gtk_toggle_tool_button_new_from_stock (GTK_STOCK_MEDIA_PLAY);
  st->playpause = GTK_WIDGET (item);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  /* Next.  */
  item = gtk_tool_button_new_from_stock (GTK_STOCK_MEDIA_NEXT);
  st->next = GTK_WIDGET (item);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  /* Random.  */
  /* XXX Need a more appropriate image.  */
  item = gtk_toggle_tool_button_new_from_stock (GTK_STOCK_REFRESH);
  st->random = GTK_WIDGET (item);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  /* Scale.  */
  item = gtk_tool_item_new ();
  gtk_tool_item_set_expand (item, TRUE);

  GtkBox *hbox = GTK_BOX (gtk_hbox_new (FALSE, 5));
  gtk_container_add (GTK_CONTAINER (item), GTK_WIDGET (hbox));
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  st->position = GTK_LABEL (gtk_label_new ("0:00"));
  gtk_label_set_width_chars (st->position, 5);
  gtk_misc_set_alignment (GTK_MISC (st->position), 1.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (st->position),
		      FALSE, FALSE, 0);

  st->scale = gtk_hscale_new_with_range (0, 100, 1);
  gtk_scale_set_draw_value (GTK_SCALE (st->scale), FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (st->scale),
		      TRUE, TRUE, 0);
    
  st->duration = GTK_LABEL (gtk_label_new ("0:00"));
  gtk_label_set_width_chars (st->duration, 5);
  gtk_misc_set_alignment (GTK_MISC (st->duration), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (st->duration),
		      FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 5);

  st->title = gtk_label_new (_("Not playing"));
  gtk_misc_set_alignment (GTK_MISC (st->duration), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), st->title, FALSE, FALSE, 0);

  st->treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (st->pl));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (st->treeview), FALSE);
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (st->treeview), TRUE);

  GtkTreeViewColumn *col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (GTK_TREE_VIEW (st->treeview), col);
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "cell-background", "gray", NULL);
  gtk_tree_view_column_pack_start (col, renderer, FALSE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (col),
				      renderer,
				      title_data_func,
				      NULL, NULL);
    
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
    
  gtk_container_add (GTK_CONTAINER (scrolled), st->treeview);
  gtk_container_add (GTK_CONTAINER (vbox), scrolled);
    
  l_button_box = gtk_hbox_new (TRUE, 0);
  create_button (&st->add, GTK_STOCK_ADD, l_button_box);
  create_button (&st->remove, GTK_STOCK_REMOVE, l_button_box);
  separator = gtk_vseparator_new();
  gtk_box_pack_start (GTK_BOX (l_button_box), separator, FALSE, FALSE, 0);
    
  create_button (&st->up, GTK_STOCK_GO_UP, l_button_box);
  create_button (&st->down, GTK_STOCK_GO_DOWN, l_button_box);
  separator = gtk_vseparator_new();
  gtk_box_pack_start (GTK_BOX (l_button_box), separator, FALSE, FALSE, 0);
    
  create_button (&st->clear, GTK_STOCK_NEW, l_button_box);
  create_button (&st->save, GTK_STOCK_SAVE_AS, l_button_box);
    
  gtk_box_pack_start (GTK_BOX (vbox), l_button_box, FALSE, FALSE, 0);
    
  st->notebook = gtk_notebook_new ();
  gtk_notebook_append_page (GTK_NOTEBOOK (st->notebook), vbox,
			    gtk_label_new (_("Player")));

  gtk_container_add (GTK_CONTAINER (main_box), GTK_WIDGET (st->notebook));

  /* Lyrics tab */

  vbox = gtk_vbox_new (FALSE, 5);
  st->textview = gtk_text_view_new ();
  GTK_TEXT_VIEW (st->textview)->editable = FALSE;

  scrolled = gtk_scrolled_window_new (NULL, NULL);
    
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);

  gtk_container_add (GTK_CONTAINER (scrolled), st->textview);

  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 3);

  gtk_notebook_append_page (GTK_NOTEBOOK (st->notebook), vbox,
			    gtk_label_new (_("Lyrics")));

  /* Web services tab (for now, last.fm) */

  hbox1 = gtk_hbox_new (FALSE, 2);
  label = gtk_label_new (_("Username:"));
  st->webuser_entry = gtk_entry_new ();

  gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox1), st->webuser_entry, TRUE, TRUE, 0);
    
  hbox2 = gtk_hbox_new (FALSE, 2);
  label = gtk_label_new (_("Password:"));
  st->webpasswd_entry = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (st->webpasswd_entry), FALSE);
    
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), st->webpasswd_entry, TRUE, TRUE, 0);
    
  /*gtk_box_pack_start (GTK_BOX (hbox), st->webuser_entry, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  */
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox1, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);

  st->web_count = gtk_label_new ("");
  st->web_submit = gtk_button_new_with_label (_("Submit"));
  gtk_box_pack_start (GTK_BOX (vbox), st->web_count, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), st->web_submit, FALSE, FALSE, 0);
    
  gtk_notebook_append_page (GTK_NOTEBOOK (st->notebook), vbox,
			    gtk_label_new (_("last.fm")));


  /* Init some variables */

    st->scale_pressed = FALSE;

    /* Just make sure */
    st->fs = NULL;
    st->fs_last_path = NULL;

    st->current_length = -1;
    st->has_lyrics = FALSE;
    st->enqueued = FALSE;
    
    callbacks_setup (st);

    lyrics_init ();

    lastfm_init (st);

    config_init (st);

    config_load (st);
    
    gtk_widget_show_all (st->window);
    
    gtk_main();
    
    return 0;
}
