/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <libintl.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>
#include <gpe/gpewindowlist.h>
#include <gpe/popup.h>

#include <gpe/launch.h>
#include <gpe/desktop_file.h>

#include <locale.h>

#include "config.h"

#define _(_x) gettext (_x)

GtkWidget *dialog;

GtkListStore *all_apps, *selected_apps;
GtkWidget *add_button, *remove_button;

char *
find_icon (char *base)
{
  char *temp;
  int i;
  
  char *directories[]=
    {
      "/usr/share/gpe/overrides",
      "/usr/share/pixmaps",
      "/usr/share/icons",
      NULL
    };
  
  if (!base)
    return NULL;

  if (base[0] == '/')
    return (char *) strdup (base);
  
  for (i=0;directories[i];i++)
    {
      temp = g_strdup_printf ("%s/%s", directories[i], base);
      if (!access (temp, R_OK))
	return temp;

      g_free (temp);

      temp = g_strdup_printf ("%s/%s.png", directories[i], base);
      if (!access (temp, R_OK))
	return temp;

      g_free (temp);
    }

  return NULL;
}

static void 
add_one_package (GnomeDesktopFile *d, gchar *name)
{
  gchar *title = NULL;
  gchar *icon_fn = NULL;
  GdkPixbuf *pixbuf = NULL;
  GtkTreeIter iter;
  gchar *p;

  p = strstr (name, ".desktop");
  if (p)
    *p = 0;
  
  gnome_desktop_file_get_string (d, NULL, "Name", &title);
  gnome_desktop_file_get_string (d, NULL, "Icon", &icon_fn);

  if (icon_fn)
    {
      gchar *full_icon_fn = find_icon (icon_fn);
      if (full_icon_fn)
	{
	  pixbuf = gdk_pixbuf_new_from_file (full_icon_fn, NULL);
	  g_free (full_icon_fn);
	}
    }

  gtk_list_store_append (all_apps, &iter);
  gtk_list_store_set (all_apps, &iter, 0, g_strdup (name), 1, title, 2, pixbuf, -1);
}

static void
load_from (const char *path)
{
  DIR *dir;
  struct dirent *entry;

  dir = opendir (path);
  if (dir)
    {
      while ((entry = readdir (dir)))
	{
	  char *temp;
	  GnomeDesktopFile *p;
	  GError *err = NULL;
	  
	  if (entry->d_name[0] == '.')
	    continue;
	
	  temp = g_strdup_printf ("%s/%s", "/usr/share/gpe/overrides", entry->d_name);
	  if (access (temp, R_OK) != 0)
	    {
	      g_free (temp);
	      temp = g_strdup_printf ("%s/%s", path, entry->d_name);
	    }
	  p = gnome_desktop_file_load (temp, &err);
	  if (p)
	    {
	      gchar *type;
	      gnome_desktop_file_get_string (p, NULL, "Type", &type);

	      if (type == NULL || strcmp (type, "Application"))
		gnome_desktop_file_free (p);
	      else
		add_one_package (p, entry->d_name);

	      if (type)
		g_free (type);
	    }
	  else
	    fprintf (stderr, "Couldn't load \"%s\": %s\n", temp, err->message);

	  if (err)
	    g_error_free (err);

	  g_free (temp);
	}

      closedir (dir);
    }
}

void
save_and_quit (void)
{
  GtkTreeIter iter;
  FILE *fp;
  gchar *path;

  path = g_strdup_printf ("%s/.gpe/button-box.apps", g_get_home_dir ());
  fp = fopen (path, "w");
  if (fp)
    {
      if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (selected_apps), &iter))
	{
	  do
	    {
	      gchar *name;
	      gtk_tree_model_get (GTK_TREE_MODEL (selected_apps), &iter, 0, &name, -1);
	      fputs (name, fp);
	      fputc ('\n', fp);
	    } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (selected_apps), &iter));
	}
      fclose (fp);
    }
  g_free (path);

  system ("killall -HUP gpe-buttonbox");

  gtk_main_quit ();
}

void
add_clicked (GtkWidget *widget, GtkTreeSelection *sel)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  int n = 0;

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      gchar *title, *name;
      GdkPixbuf *pix;

      gtk_tree_model_get (model, &iter, 0, &name, 1, &title, 2, &pix, -1);
 
      if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (selected_apps), &iter))
	{
	  do
	    {
	      gchar *this_name;
	      gtk_tree_model_get (GTK_TREE_MODEL (selected_apps), &iter, 0, &this_name, -1);
	      if (!strcmp (this_name, name))
		{
		  gpe_error_box ("This application is already selected");
		  return;
		}
	      n++;
	    } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (selected_apps), &iter));
	}
      
      if (n == 6)
	{
	  gpe_error_box ("No more than six buttons may be defined");
	  return;
	}

      gtk_list_store_append (GTK_LIST_STORE (selected_apps), &iter);
      gtk_list_store_set (GTK_LIST_STORE (selected_apps), &iter, 0, name, 1, title, 2, pix, -1);
    }
}

void
delete_clicked (GtkWidget *widget, GtkTreeSelection *sel)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
}

void
sensitize (GtkTreeSelection *sel, GtkWidget *widget)
{
  gtk_widget_set_sensitive (widget, gtk_tree_selection_get_selected (sel, NULL, NULL));
}

void
select_item (gchar *name)
{
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (all_apps), &iter))
    {
      do
	{
	  gchar *this_name;
	  gtk_tree_model_get (GTK_TREE_MODEL (all_apps), &iter, 0, &this_name, -1);
	  if (!strcmp (name, this_name))
	    {
	      gchar *title;
	      GdkPixbuf *pix;
	      gtk_tree_model_get (GTK_TREE_MODEL (all_apps), &iter, 1, &title, 2, &pix, -1);
	      gtk_list_store_append (selected_apps, &iter);
	      gtk_list_store_set (selected_apps, &iter, 0, name, 1, title, 2, pix, -1);
	      return;
	    }
	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (all_apps), &iter));
    }
}

void
read_selected (void)
{
  gchar *path;
  FILE *fp;

  path = g_strdup_printf ("%s/.gpe/button-box.apps", g_get_home_dir ());
  fp = fopen (path, "r");
  if (fp == NULL)
    fp = fopen ("/etc/gpe/button-box.apps", "r");
  if (fp)
    {
      char buf[256];
      while (!feof (fp))
	{
	  if (fgets (buf, sizeof (buf) - 1, fp))
	    {
	      buf[strlen (buf) - 1] = 0;
	      select_item (buf);
	    }
	}
    }

  g_free (path);
}

int
main (int argc, char *argv[])
{
  GtkWidget *hbox;
  GtkWidget *scrolled;
  GtkWidget *selected_apps_box;
  GtkWidget *all_apps_box;
  GtkWidget *left_vbox, *right_vbox;
  GtkWidget *left_label, *right_label;
  GtkWidget *ok_button, *cancel_button;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeSelection *selection;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  dialog = gtk_dialog_new ();

  hbox = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);

  all_apps = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF);
  selected_apps = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF);

  left_vbox = gtk_vbox_new (FALSE, 2);
  right_vbox = gtk_vbox_new (FALSE, 2);

  all_apps_box = gtk_tree_view_new_with_model (GTK_TREE_MODEL (all_apps));
  selected_apps_box = gtk_tree_view_new_with_model (GTK_TREE_MODEL (selected_apps));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (all_apps_box), FALSE);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (selected_apps_box), FALSE);

  gtk_tree_view_set_reorderable (GTK_TREE_VIEW (selected_apps_box), TRUE);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled), all_apps_box);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes ("Icon", renderer, "pixbuf", 2, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (all_apps_box), column);

  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes ("Icon", renderer, "pixbuf", 2, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (selected_apps_box), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Name", renderer, "text", 1, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (all_apps_box), column);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Name", renderer, "text", 1, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (selected_apps_box), column);
  
  left_label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (left_label), "<b>Available programs</b>");
  right_label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (right_label), "<b>Selected programs</b>");

  add_button = gtk_button_new_from_stock (GTK_STOCK_ADD);
  remove_button = gtk_button_new_from_stock (GTK_STOCK_REMOVE);

  gtk_box_pack_start (GTK_BOX (left_vbox), left_label, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (right_vbox), right_label, FALSE, FALSE, 2);

  gtk_box_pack_start (GTK_BOX (left_vbox), scrolled, TRUE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (right_vbox), selected_apps_box, TRUE, TRUE, 2);

  gtk_box_pack_start (GTK_BOX (left_vbox), add_button, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (right_vbox), remove_button, FALSE, FALSE, 2);

  gtk_box_pack_start (GTK_BOX (hbox), left_vbox, TRUE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), right_vbox, TRUE, TRUE, 2);

  gtk_window_set_default_size (GTK_WINDOW (dialog), 640, 480);

  ok_button = gtk_button_new_from_stock (GTK_STOCK_OK);
  cancel_button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);

  g_signal_connect (G_OBJECT (dialog), "delete-event", G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (G_OBJECT (cancel_button), "clicked", G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (G_OBJECT (ok_button), "clicked", G_CALLBACK (save_and_quit), NULL);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (all_apps_box));
  sensitize (selection, add_button);
  g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (sensitize), add_button);
  g_signal_connect (G_OBJECT (add_button), "clicked", G_CALLBACK (add_clicked), selection);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (selected_apps_box));
  sensitize (selection, remove_button);
  g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (sensitize), remove_button);
  g_signal_connect (G_OBJECT (remove_button), "clicked", G_CALLBACK (delete_clicked), selection);

  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dialog)->action_area), cancel_button, FALSE, FALSE, 2);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dialog)->action_area), ok_button, FALSE, FALSE, 2);

  load_from ("/usr/share/applications");

  read_selected ();

  gtk_widget_show_all (dialog);

  gtk_main ();

  return 0;  
}
