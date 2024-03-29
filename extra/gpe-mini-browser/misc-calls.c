/*
 * gpe-mini-browser v0.21
 *
 * Basic web browser based on gtk-webcore 
 * 
 * Misc calls.
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


#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gdk/gdk.h>

#include <glib.h>
#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/question.h>
#include <gpe/gpedialog.h>
#include <gpe/picturebutton.h>

#include "gpe-mini-browser.h"

#ifdef HILDON
/* Hildon includes */
#include <hildon-widgets/hildon-app.h>
#include <hildon-widgets/hildon-appview.h>
#include <libosso.h>
#endif /*HILDON*/
//#define DEBUG /* uncomment this if you want debug output*/
#ifndef NOBOOKMARKS
  void
delete_bookmarks (GtkWidget * button, gpointer * data)
{
  if (gpe_question_ask
      (_("Really delete this bookmark or category?"), "Delete?", "question",
       "!gtk-cancel", NULL, "!gtk-delete", NULL, NULL) == 1)
    {
      GtkTreeSelection *selection;
      GtkTreeModel *model;
      GtkTreeIter iter;
      gchar *url;

      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data));
      model = gtk_tree_view_get_model (GTK_TREE_VIEW (data));

      if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
	  gtk_tree_model_get (model, &iter, 0, &url, -1);
#ifdef DEBUG
	  printf ("Deleting url %s\n", url);
#endif

	  /* delete from db */
	  int err = remove_bookmark (url);
	  if (err)
	    gpe_error_box (_("error removing bookmark from db!\n"));
	  /* delete from list */
	  gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
	}
    }
}

#endif /* NOBOOKMARKS */
/*==============================================*/
void
open_bookmarks (GtkWidget * button, gpointer * data)
{
  struct tree_action *open_data;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *url;

  open_data = (struct tree_action *) data;

  selection =
    gtk_tree_view_get_selection (GTK_TREE_VIEW (open_data->treeview));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (open_data->treeview));

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, 0, &url, -1);
      url = (gchar *) parse_url (url);
#ifdef DEBUG
      printf ("The selected url = %s\n", url);
#endif
      /* add to history */
      add_history (url);

      fetch_url (url, GTK_WIDGET (open_data->html));
      g_free (url);
    }
  else
    {
      gpe_error_box (_("No bookmark selected!"));
    }
}

/*==============================================*/
#ifndef NOBOOKMARKS
void
toggle_type (GtkWidget * button, gpointer * data)
{
  struct bookmark_add *test;

  test = (struct bookmark_add *) data;

  if (test->bookmark_type)
    test->bookmark_type = 0;
  else
    test->bookmark_type = 1;
}

/*==============================================*/

void
add_bookmark_func (GtkWidget * button, gpointer * data)
{
  struct bookmark_add *info;
  GtkTreeModel *model;
  GtkTreeIter iter;
  const gchar *location;

  info = (struct bookmark_add *) data;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (info->treeview));

  if (info->bookmark_type == 0)
    {
      location = webi_get_location (info->html);
    }
  else
    {
      location = gtk_editable_get_chars (GTK_EDITABLE (info->entry), 0, -1);
    }
#ifdef DEBUG
  printf ("bookmark value is %s\n", location);
#endif
  if (location != NULL)
    {
      // add to database
      int err = insert_new_bookmark ((char *) location);
      if (err)
	gpe_error_box (_("Error accessing db!\n"));
      gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
      gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 0, location, -1);
    }
  else
    {
      gpe_error_box (_("No url filled in!\n"));
    }


}
#endif /* NOBOOKMARKS */
/*==============================================*/

int
set_entry_completion (GtkWidget * entry)
{
  GtkTreeModel *model;
  GtkEntryCompletion *completion;

  model = create_completion_model ();
  if (model == NULL)
    return 0;

  completion = gtk_entry_completion_new ();
  gtk_entry_completion_set_model (completion, model);
  gtk_entry_completion_set_text_column (completion, 0);
  gtk_entry_completion_set_minimum_key_length (completion, 3);
  gtk_entry_set_completion (GTK_ENTRY (entry), completion);

  /*default should be replacing the contents of the entry by the selected content */

  return 1;
}

/*==============================================*/

GtkTreeModel *
create_completion_model (void)
{

  gint count = 0;
  GtkTreeIter iter;
  FILE *file;
  char *buf;
  char buffer[64];

  buf = history_location ();
  if (!completion_store)
    completion_store = gtk_list_store_new (1, G_TYPE_STRING);

  file = fopen (buf, "ro");
  /* if file does not exist just start and skip reading in data */
  if (!file)
    return GTK_TREE_MODEL (completion_store);;

  while (fgets (buffer, 63, file))
    {
      buffer[strlen (buffer) - 1] = 0;

      gtk_list_store_append (completion_store, &iter);
      gtk_list_store_set (completion_store, &iter, 0, buffer, -1);

      count++;
      if (count == 100)
	break;
    }

  fclose (file);
  g_free (buf);

  return GTK_TREE_MODEL (completion_store);
}

/*==============================================*/

void
save_completion (GtkWidget * window)
{
  gint count = 0;
  GtkTreeIter iter;
  FILE *file;
  char *buf;
  gchar *buffer = NULL;

  buf = history_location ();

  file = fopen (buf, "w");

  if (!gtk_tree_model_get_iter_first
      (GTK_TREE_MODEL (completion_store), &iter))
    goto end;

  while (count < 100)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (completion_store), &iter, 0,
			  &buffer, -1);
      count++;
      fprintf (file, "%s\n", buffer);
      g_free (buffer);
      if (!gtk_tree_model_iter_next
	  (GTK_TREE_MODEL (completion_store), &iter))
	goto end;
    }
end:fclose (file);
  g_free (buf);
}

/*==============================================*/

gchar *
history_location (void)
{
  const char *home = getenv ("HOME");
  char *buf;
  size_t len;

  if (home == NULL)
    home = "";
  len = strlen (home) + strlen (COMPLETION) + 1;
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, COMPLETION);

  return buf;
}

/*==============================================*/

void
clear_history (GtkWidget * button)
{
  char *filename;
  FILE *file;

  filename = history_location ();

  file = fopen (filename, "w");
  fclose (file);
  gtk_list_store_clear (completion_store);
}

/*==============================================*/

void
set_fullscreen (GtkWidget * button, gpointer * fullscreen_info)
{
  static int fullscreen;
  static int totalscreen;
  struct fullscreen_info *info;
  static GtkWidget *close_button, *fullscreen_popup;

  info = (struct fullscreen_info *) fullscreen_info;

  if (!fullscreen)
    {
#ifdef HILDON
      hildon_appview_set_fullscreen (HILDON_APPVIEW (info->app), TRUE);
#else
      gtk_window_fullscreen (GTK_WINDOW (info->app));
#endif
      fullscreen = 1;
    }
  else if (fullscreen && !totalscreen)
    {

      totalscreen = 1;
#ifdef HILDON
      gtk_widget_hide (info->hsep);
#endif
      gtk_widget_hide (info->toolbar);
      if (info->urlbox)
	gtk_widget_hide (info->urlbox);
      fullscreen_popup = gtk_window_new (GTK_WINDOW_POPUP);

      close_button =
	gpe_button_new_from_stock (GTK_STOCK_ZOOM_FIT, GPE_BUTTON_TYPE_ICON);
      g_signal_connect (G_OBJECT (close_button), "clicked",
			G_CALLBACK (set_fullscreen), fullscreen_info);
      gtk_container_add (GTK_CONTAINER (fullscreen_popup), close_button);
      gtk_widget_show_all (fullscreen_popup);

    }
  else if (totalscreen == 1 && fullscreen == 1)
    {
#ifdef HILDON
      hildon_appview_set_fullscreen (HILDON_APPVIEW (info->app), FALSE);
      gtk_widget_show (info->hsep);
#else
      gtk_window_unfullscreen (GTK_WINDOW (info->app));
#endif
      gtk_widget_destroy (GTK_WIDGET (fullscreen_popup));
      gtk_widget_show_all (info->toolbar);
      if (!urlbar_hidden)
	if (info->urlbox)
	  gtk_widget_show_all (info->urlbox);
      fullscreen = 0;
      totalscreen = 0;
    }
}

/*==============================================*/

void
set_as_homepage (GtkWidget * button, gpointer * data)
{
  struct tree_action *open_data;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *url;
  GString *info;

  open_data = (struct tree_action *) data;

  selection =
    gtk_tree_view_get_selection (GTK_TREE_VIEW (open_data->treeview));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (open_data->treeview));

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, 0, &url, -1);
#ifdef DEBUG
      printf ("The *new* home url = %s\n", url);
#endif
      info = g_string_new ("The new home url is \0");
      info = g_string_append (info, url);
      gpe_info_dialog (info->str);
      set_bookmark_home (url);
      g_free (url);
      g_string_free (info, TRUE);
    }
  else
    {
      gpe_error_box (_("No bookmark selected!"));
    }
}

/*==============================================*/

void 
add_history (gchar *url)
{
  GtkTreeIter iter, hist_iter;
  int count = 0;
  gchar *buffer;

  if (!gtk_tree_model_get_iter_first
      (GTK_TREE_MODEL (completion_store), &iter))
  {
#ifdef DEBUG
   printf("get first iter for history failed\n");
#endif 
  }
  else
  { 
   while (count < 100)
     {
      gtk_tree_model_get (GTK_TREE_MODEL (completion_store), &iter, 0,
                          &buffer, -1);
      count++;
      if(strcmp (buffer, url)== 0)
	{
#ifdef DEBUG
	 printf ("url: %s already in history!\n", url);
#endif 
      	 g_free (buffer);
	 return;
	}
      if (!gtk_tree_model_iter_next
          (GTK_TREE_MODEL (completion_store), &iter))
	{
	 break;
        }
     }
   /* in case the list is 100 items long, remove the last one to make room for a new one */
   if(count == 100)
	 gtk_list_store_remove (completion_store, &iter);
   g_free (buffer);
  }
  
  gtk_list_store_insert (completion_store, &hist_iter, 0);
  gtk_list_store_set (completion_store, &hist_iter, 0, url, -1);

  return;
}
