/*
 * gpe-mini-browser v0.17
 *
 * Basic web browser based on gtk-webcore 
 * 
 * Interface calls.
 *
 * Copyright (c) 2005 Philippe De Swert
 *
 * Contact : philippedeswert@scarlet.be
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
 */

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <webi.h>

#include <gdk/gdk.h>

#include <glib.h>

#include <gpe/init.h>
#include "gpe/pixmaps.h"
#include "gpe/init.h"
#include "gpe/picturebutton.h"
#include <gpe/errorbox.h>
#include <gpe/spacing.h>

#include "gpe-mini-browser.h"

//#define DEBUG /* uncomment this if you want debug output*/

/* ======================================================== */

/* pop up a window to enter an URL */
void
show_url_window (GtkWidget * show, GtkWidget * html)
{
  GtkWidget *url_window, *entry;
  GtkWidget *hbox, *vbox;
  GtkWidget *label;
  GtkWidget *buttonok, *buttoncancel;
  struct url_data *data;

  /* create dialog window */
  url_window = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (url_window), _("Where to go to?"));

  hbox = gtk_hbox_new (FALSE, 0);
  entry = gtk_entry_new ();
  /* set completion on url entry */
  set_entry_completion(entry);
 
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  label = gtk_label_new (_("Enter url:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  gtk_container_set_border_width (GTK_CONTAINER (url_window),
				  gpe_get_border ());

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (url_window)->vbox),
		      hbox, FALSE, FALSE, 0);

  /* add the buttons */
  buttonok = gpe_button_new_from_stock (GTK_STOCK_OK, GPE_BUTTON_TYPE_BOTH);
  buttoncancel =
    gpe_button_new_from_stock (GTK_STOCK_CANCEL, GPE_BUTTON_TYPE_BOTH);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (url_window)->action_area),
		      buttoncancel, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (url_window)->action_area),
		      buttonok, TRUE, TRUE, 0);

  GTK_WIDGET_SET_FLAGS (buttonok, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (buttonok);

  data = malloc (sizeof (struct url_data));
  data->html = html;
  data->entry = entry;
  data->window = url_window;

  /* add button callbacks */
  g_signal_connect (GTK_OBJECT (buttonok), "clicked",
		    G_CALLBACK (load_text_entry), (gpointer *) data);
  g_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
		    G_CALLBACK (destroy_window), (gpointer *) url_window);
  g_signal_connect (GTK_OBJECT (buttonok), "clicked",
		    G_CALLBACK (destroy_window), (gpointer *) url_window);
  g_signal_connect (GTK_OBJECT (entry), "activate",
		    G_CALLBACK (load_text_entry), (gpointer *) data);
  g_signal_connect (GTK_OBJECT (entry), "activate",
		    G_CALLBACK (destroy_window), (gpointer *) url_window);

  gtk_widget_show_all (url_window);
  gtk_widget_grab_focus (entry);
}

/* ======================================================== */

void
destroy_window (GtkButton * button, gpointer * window)
{
  gtk_widget_destroy (GTK_WIDGET (window));
}

/* ======================================================== */

void
create_status_window (Webi * html, gpointer * status_data)
{
  GtkWidget *statusbox, *pbar, *label;
  struct status_data *data;

  data = (struct status_data *) status_data;
  /* the stop signal is not always generated. Like when you click on a link in a page
     before it is fully loaded. So to avoid several status bars to appear (and not all 
     disappearing), check if there is already one and remoce it. */
  if (data->exists == TRUE)
    {
      gtk_widget_destroy (GTK_WIDGET (data->statusbox));
    }

#ifdef DEBUG
  printf ("status = loading\n");
#endif
  gtk_tool_button_set_stock_id (GTK_TOOL_BUTTON (stop_reload_button),
				"gtk-stop");
  gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (stop_reload_button),
				   NULL);

  statusbox = gtk_hbox_new (FALSE, 0);
  pbar = gtk_progress_bar_new ();
  label = gtk_label_new (_("loading"));

  data->statusbox = statusbox;
  data->pbar = pbar;
  data->exists = TRUE;

  gtk_box_pack_end (GTK_BOX (data->main_window), statusbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (statusbox), label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (statusbox), GTK_WIDGET (pbar), FALSE, FALSE,
		      0);

  gtk_widget_show (statusbox);
  gtk_widget_show (GTK_WIDGET (pbar));
  gtk_widget_show (label);
}

/* ======================================================== */

void
destroy_status_window (Webi * html, gpointer * status_data)
{
  struct status_data *data;

  data = (struct status_data *) status_data;

#ifdef DEBUG
  printf ("loading stopped\n");
#endif
  gtk_tool_button_set_stock_id (GTK_TOOL_BUTTON (stop_reload_button),
				"gtk-refresh");
  gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (stop_reload_button),
				   NULL);

  if (data->exists == TRUE)
    {
      data->exists = FALSE;
      /* set pbar to NULL for testing in activate_statusbar (we do not want to access a destroyed widget) */
      data->pbar = NULL;
      gtk_widget_destroy (GTK_WIDGET (data->statusbox));
    }
}

/* ======================================================== */

void
activate_statusbar (Webi * html, WebiLoadStatus * status,
		    gpointer status_data)
{
  gdouble fraction = 0.0;
  struct status_data *data;

#ifdef DEBUG
  printf ("progressbar status changed\n");
#endif

  data = (struct status_data *) status_data;

  /* test if an error occured */
  if (status->status == WEBI_LOADING_ERROR)
    {
      gpe_error_box (_("An error occured loading the webpage!"));
    }

  /* copied from the reference implementation of osb-browser, needs to be improved for this app */
  if (status->filesWithSize < status->files)
    {
      gdouble ratio =
	(gdouble) status->filesWithSize / (gdouble) status->files;
      fraction +=
	status->received >=
	status->size ? ratio : ratio * (gdouble) status->received /
	(gdouble) status->size;
      fraction +=
	status->ready >=
	status->files ? (1.0 - ratio) : (1.0 -
					 ratio) * (gdouble) (status->ready -
							     status->
							     filesWithSize) /
	(gdouble) (status->files - status->filesWithSize);
    }
  else
    {
      fraction = (gdouble) status->received / (gdouble) status->size;
    }
  if (fraction > 1.0)
    {
      fraction = 1.0;
    }
  /* see if the widget still exists */
  if (data->pbar != NULL)
    gtk_progress_bar_set_fraction ((GtkProgressBar *) data->pbar, fraction);
}

/* ======================================================== */

void
set_title (Webi * html, GtkWidget * app_window)
{
  const gchar *title;

  title = webi_get_title (html);
  title = g_strconcat (title, " - mini-browser", NULL);
  gtk_window_set_title (GTK_WINDOW (app_window), title);
  g_free ((gpointer *) title);

}

/* ======================================================== */

void
update_text_entry (Webi * html, GtkWidget * entrybox)
{
  const gchar *location;

  location = webi_get_location (html);
  if (entrybox != NULL)
    gtk_entry_set_text (GTK_ENTRY (entrybox), location);

}

/* ======================================================== */

GtkWidget *
show_big_screen_interface (Webi * html, GtkWidget * toolbar,
			   WebiSettings * set)
{
  GtkWidget *urlbox;
  GtkToolItem *sep, *sep2, *hide_button;
  static struct urlbar_data hide;

  urlbox = create_url_bar (html);

  sep = gtk_separator_tool_item_new ();
  gtk_tool_item_set_homogeneous (sep, FALSE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), sep, -1);

  hide_button = gtk_tool_button_new_from_stock (GTK_STOCK_UNDO);
  gtk_tool_item_set_homogeneous (hide_button, FALSE);
  gtk_tool_button_set_label (GTK_TOOL_BUTTON (hide_button), _("Hide Url"));
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), hide_button, -1);

  sep2 = gtk_separator_tool_item_new ();
  gtk_tool_item_set_homogeneous (sep2, FALSE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), sep2, -1);

  add_zoom_buttons(html, toolbar, set);

  /* fill in info for hiding */

  hide.urlbox = urlbox;
  hide.hidden = 0;		/* when hidden this value will be 1 */
  hide.hiding_button = hide_button;

  /* add button callbacks */
  g_signal_connect (GTK_OBJECT (hide_button), "clicked",
		    G_CALLBACK (hide_url_bar), &hide);

  return (urlbox);
}

/* ======================================================== */

void
add_zoom_buttons (Webi *html, GtkWidget *toolbar, WebiSettings * set)
{

  GtkToolItem *zoom_in_button, *zoom_out_button, *sep;


/* fill in the data needed for the zoom functionality */
  static struct zoom_data zoom;

  zoom.html = html;
  zoom.settings = set;

  /* add extra zoom in/out buttons + spacing to the toolbar */

  zoom_in_button = gtk_tool_button_new_from_stock (GTK_STOCK_ZOOM_IN);
  gtk_tool_item_set_homogeneous (zoom_in_button, FALSE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), zoom_in_button, -1);

  zoom_out_button = gtk_tool_button_new_from_stock (GTK_STOCK_ZOOM_OUT);
  gtk_tool_item_set_homogeneous (zoom_out_button, FALSE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), zoom_out_button, -1);

  sep = gtk_separator_tool_item_new ();
  gtk_tool_item_set_homogeneous (sep, FALSE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), sep, -1);

  /* add button callbacks */
  g_signal_connect (GTK_OBJECT (zoom_in_button), "clicked",
		    G_CALLBACK (zoom_in), &zoom);
  g_signal_connect (GTK_OBJECT (zoom_out_button), "clicked",
		    G_CALLBACK (zoom_out), &zoom);

}

/* ======================================================== */

GtkWidget *
create_url_bar (Webi * html)
{
  static struct url_data data;
  GtkWidget *urlbox, *urllabel, *urlentry, *okbutton;

  /* create all necessary widgets */
  urlbox = gtk_hbox_new (FALSE, 0);
  urllabel = gtk_label_new (_(" Url:"));
  gtk_misc_set_alignment (GTK_MISC (urllabel), 0.0, 0.5);
  urlentry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (urlentry), TRUE);
  okbutton = gpe_button_new_from_stock (GTK_STOCK_OK, GPE_BUTTON_TYPE_BOTH);

  /* set completion on url entry */
  set_entry_completion(urlentry);

  /* pack everything in the hbox */
  gtk_box_pack_start (GTK_BOX (urlbox), urllabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (urlbox), urlentry, TRUE, TRUE, 5);
  gtk_box_pack_start (GTK_BOX (urlbox), okbutton, FALSE, FALSE, 10);

  data.html = (GtkWidget *) html;
  data.entry = urlentry;
  data.window = NULL;		/* set to NULL to be easy to recognize to avoid freeing in load_text_entry  as this window is not destroyed unlike the pop-up */

  g_signal_connect (GTK_OBJECT (okbutton), "clicked",
		    G_CALLBACK (load_text_entry), &data);
  g_signal_connect (GTK_OBJECT (html), "location",
		    G_CALLBACK (update_text_entry), (gpointer *) urlentry);
  g_signal_connect (GTK_OBJECT (urlentry), "activate",
		    G_CALLBACK (load_text_entry), &data);

  gtk_widget_grab_focus (urlentry);
  /*final settings */
  GTK_WIDGET_SET_FLAGS (okbutton, GTK_CAN_DEFAULT);
  gtk_button_set_relief (GTK_BUTTON (okbutton), GTK_RELIEF_NONE);
  gtk_widget_grab_default (okbutton);

  return (urlbox);
}

/* ======================================================== */

void
hide_url_bar (GtkWidget * button, struct urlbar_data *url_bar)
{
  struct urlbar_data *data = NULL;

  data = (struct urlbar_data *) url_bar;

  if (data->hidden != 1)
    {
      gtk_widget_hide (data->urlbox);
      data->hidden = 1;
      gtk_tool_button_set_stock_id (GTK_TOOL_BUTTON (data->hiding_button), "gtk-redo");
      gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (data->hiding_button), NULL);
      gtk_tool_button_set_label (GTK_TOOL_BUTTON (data->hiding_button), _("Show Url"));
    }
  else
    {
      gtk_widget_show_all (data->urlbox);
      data->hidden = 0;
      gtk_tool_button_set_stock_id (GTK_TOOL_BUTTON (data->hiding_button), "gtk-undo");
      gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (data->hiding_button), NULL);
      gtk_tool_button_set_label (GTK_TOOL_BUTTON (data->hiding_button), _("Hide Url"));
    }

}

/* ======================================================== */

#ifndef NOBOOKMARKS
void
show_bookmarks (GtkWidget * button, Webi * html)
{
  GtkWidget *bookmarks_window, *bookbox, *scroll_window, *bookmark_list;
  GtkToolbar *booktool;
  GtkToolItem *add, *del, *open_bookmark;
  GtkTreeModel *model;
  GtkCellRenderer *cell;
  GtkTreeViewColumn *column;
  static struct tree_action tree_info;
  GtkTreeSelection *selection;

  bookmarks_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (bookmarks_window), _("Bookmarks"));
  gtk_window_set_default_size (GTK_WINDOW (bookmarks_window), 240, 320);
  gtk_window_set_type_hint (GTK_WINDOW(bookmarks_window), GDK_WINDOW_TYPE_HINT_DIALOG); 
  gtk_window_set_decorated (GTK_WINDOW(bookmarks_window), TRUE);

  bookbox = gtk_vbox_new (FALSE, 0);
  booktool = GTK_TOOLBAR (gtk_toolbar_new ());

  add = gtk_tool_button_new_from_stock (GTK_STOCK_ADD);
  gtk_tool_item_set_homogeneous (add, FALSE);
  gtk_toolbar_insert (booktool, add, -1);

  del = gtk_tool_button_new_from_stock (GTK_STOCK_REMOVE);
  gtk_tool_item_set_homogeneous (del, FALSE);
  gtk_toolbar_insert (booktool, del, -1);

  open_bookmark = gtk_tool_button_new_from_stock (GTK_STOCK_OPEN);
  gtk_tool_item_set_homogeneous (open_bookmark, FALSE);
  gtk_toolbar_insert (booktool, open_bookmark, -1);

  // scrolled window with bookmark list
  scroll_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll_window),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  bookmark_list = gtk_tree_view_new ();

  // fill in the tree info struct

  tree_info.html = html;
  tree_info.treeview = bookmark_list;  

  model = GTK_TREE_MODEL(gtk_tree_store_new(1, G_TYPE_STRING));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (bookmark_list), FALSE);
  gtk_tree_view_set_model (GTK_TREE_VIEW (bookmark_list),
			   GTK_TREE_MODEL (model));
  gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(bookmark_list), TRUE);
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(bookmark_list));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

  start_db();

  refresh_or_load_db(bookmark_list);

  cell = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Bookmarks"), cell, 
						     "text", 0, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (bookmark_list),
			       GTK_TREE_VIEW_COLUMN (column));

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroll_window),
					 bookmark_list);

  // connect signals
  g_signal_connect (GTK_OBJECT (add), "clicked",
		    G_CALLBACK (show_add_dialog), &tree_info);
  g_signal_connect (GTK_OBJECT (del), "clicked",
		    G_CALLBACK (delete_bookmarks), bookmark_list);
  g_signal_connect (GTK_OBJECT (open_bookmark), "clicked",
		    G_CALLBACK (open_bookmarks), &tree_info);
  g_signal_connect (GTK_OBJECT (open_bookmark), "clicked",
		    G_CALLBACK (destroy_window), bookmarks_window);
  g_signal_connect (GTK_OBJECT (bookmarks_window), "destroy",
		    G_CALLBACK (stop_db), NULL);

  gtk_box_pack_start (GTK_BOX (bookbox), GTK_WIDGET (booktool), FALSE, FALSE,
		      0);
  gtk_box_pack_start (GTK_BOX (bookbox), GTK_WIDGET (scroll_window), TRUE,
		      TRUE, 0);
  gtk_container_add (GTK_CONTAINER (bookmarks_window), bookbox);

  gtk_widget_show_all (bookmarks_window);

}

/* ======================================================== */

void
show_add_dialog (GtkWidget * button, gpointer *data)
{
  GtkWidget *add_dialog, *add_new, *add_current, *add;
  GtkWidget *hbox, *vbox;
  GtkWidget *entry, *cancel;
  GSList *group;
  static struct bookmark_add bookmark;
  struct tree_action *tree_info;

  tree_info = (struct tree_action *) data;

  bookmark.html = tree_info->html;
  bookmark.treeview = tree_info->treeview;
  bookmark.bookmark_type = 0;

  /* new dialog window */
  add_dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (add_dialog), _("Add a bookmark"));
  gtk_container_set_border_width (GTK_CONTAINER (add_dialog),
				  gpe_get_border ());

  /* vbox for selector */
  vbox = gtk_vbox_new (FALSE, 0);
  add_current =
    gtk_radio_button_new_with_label (NULL, _("Add current web page"));

  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (add_current));

  /* hbox for add new button + text-entry */
  hbox = gtk_hbox_new (FALSE, 0);
  add_new = gtk_radio_button_new_with_label (group, _("Add new :"));

  entry = gtk_entry_new ();

  bookmark.entry = entry;
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), add_new, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);


  /* pack the buttons and the hbow with the entrybox on the dialog */
  gtk_box_pack_start (GTK_BOX (vbox), add_current, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (add_dialog)->vbox),
		      vbox, FALSE, FALSE, 0);

  /* dialog buttons */
  add = gpe_button_new_from_stock (GTK_STOCK_ADD, GPE_BUTTON_TYPE_BOTH);
  cancel = gpe_button_new_from_stock (GTK_STOCK_CANCEL, GPE_BUTTON_TYPE_BOTH);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (add_dialog)->action_area),
		      cancel, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (add_dialog)->action_area),
		      add, TRUE, TRUE, 0);

  /* connect the signals */
  g_signal_connect (GTK_OBJECT (cancel), "clicked",
		    G_CALLBACK (destroy_window), (gpointer *) add_dialog);
  /* only connect one button to the toggle signal otherwise the bookmark type get toggled twice */
  g_signal_connect (GTK_OBJECT (add_current), "toggled",
		    G_CALLBACK (toggle_type), &bookmark);
  g_signal_connect (GTK_OBJECT (add), "clicked",
		    G_CALLBACK (add_bookmark_func), &bookmark);
  g_signal_connect (GTK_OBJECT (add), "clicked",
		    G_CALLBACK (destroy_window), (gpointer *) add_dialog);

  gtk_widget_show_all (add_dialog);
  gtk_widget_grab_focus (entry);

}
#endif
