/*
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
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
#include <string.h>

#include <gtk/gtk.h>

#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/picturebutton.h>
#include <gpe/spacing.h>

#include <libdisplaymigration/displaymigration.h>

#include "todo.h"

#define _(_x) gettext(_x)

static void
ui_del_category (GtkWidget *widget,
		 gpointer user_data)
{
  GtkCList *clist = GTK_CLIST (user_data);
  if (clist->selection)
    {
      guint row = (guint)clist->selection->data;
      struct todo_category *t = gtk_clist_get_row_data (clist, row);
      todo_db_del_category (t);
      gtk_clist_remove (clist, row);
      gtk_widget_draw (GTK_WIDGET (clist), NULL);
      categories_menu ();
    }
}

static void
ui_create_new_category (GtkWidget *widget,
			GtkWidget *d)
{
  GtkWidget *entry = gtk_object_get_data (GTK_OBJECT (d), "entry");
  char *title = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
  GSList *l;
  GtkWidget *clist = gtk_object_get_data (GTK_OBJECT (d), "clist");
  gchar *line_info[1];
  guint row;
  struct todo_category *t;

  if (title[0] == 0)
    {
      gpe_error_box (_("Category name must not be blank"));
      gtk_widget_destroy (d);
      return;
    }
  
  for (l = todo_db_get_categories_list(); l; l = l->next)
    {
      struct todo_category *t = l->data;
      if (!strcmp (title, t->title))
	{
	  gpe_error_box (_("A category by that name already exists"));
	  gtk_widget_destroy (d);
	  return;
	}
    }

  t = todo_db_new_category (title);
  line_info[0] = title;
  row = gtk_clist_append (GTK_CLIST (clist), line_info);
  gtk_clist_set_row_data (GTK_CLIST (clist), row, t);
  categories_menu ();
  gtk_widget_destroy (d);
}

static void
close_window(GtkWidget *widget,
	     GtkWidget *w)
{
  gtk_widget_destroy (w);
}

static void
new_category_box (GtkWidget *w, gpointer data)
{
  GtkWidget *window = gtk_dialog_new ();
  GtkWidget *vbox = GTK_DIALOG (window)->vbox;
  GtkWidget *ok;
  GtkWidget *cancel;
  GtkWidget *label = gtk_label_new (_("Name:"));
  GtkWidget *name = gtk_entry_new ();
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  guint spacing = gpe_get_boxspacing ();

  displaymigration_mark_window (window);

  ok = gpe_button_new_from_stock (GTK_STOCK_OK, GPE_BUTTON_TYPE_BOTH);
  cancel = gpe_button_new_from_stock (GTK_STOCK_CANCEL, GPE_BUTTON_TYPE_BOTH);

  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), name, TRUE, TRUE, 2);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), cancel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), ok, FALSE, FALSE, 0);

  GTK_WIDGET_SET_FLAGS (ok, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (ok);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, spacing);

  gtk_object_set_data (GTK_OBJECT (window), "entry", name);
  gtk_object_set_data (GTK_OBJECT (window), "clist", data);
  
  g_signal_connect (G_OBJECT (ok), "clicked", G_CALLBACK (ui_create_new_category), window);
  
  g_signal_connect (G_OBJECT (cancel), "clicked", G_CALLBACK (close_window), window);

  gtk_window_set_title (GTK_WINDOW (window), _("New category"));
  gpe_set_window_icon (window, "icon");

  gtk_container_set_border_width (GTK_CONTAINER (window), gpe_get_border ());

  gtk_widget_show_all (window);
  gtk_widget_grab_focus (name);
}

void
configure (GtkWidget *w, gpointer list)
{
  GtkWidget *window = gtk_dialog_new ();
  GtkWidget *toolbar;
  GtkWidget *vbox = GTK_DIALOG (window)->vbox;
  GtkWidget *clist = gtk_clist_new (1);
  GtkWidget *okbutton;
  GSList *l;

  displaymigration_mark_window (window);

  gtk_window_set_title (GTK_WINDOW (window), _("To-do list: Categories"));
  gpe_set_window_icon (window, "icon");

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

  for (l = todo_db_get_categories_list(); l; l = l->next)
    {
      struct todo_category *t = l->data;
      gchar *line_info[1];
      guint row;
      
      line_info[0] = (gchar *)t->title;
      row = gtk_clist_append (GTK_CLIST (clist), line_info);
      gtk_clist_set_row_data (GTK_CLIST (clist), row, t);
    }

  okbutton = gtk_button_new_from_stock (GTK_STOCK_OK);
  g_signal_connect_swapped (G_OBJECT (okbutton), "clicked", G_CALLBACK (gtk_widget_destroy), window);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), okbutton, FALSE, FALSE, 0);
  GTK_WIDGET_SET_FLAGS (okbutton, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (okbutton);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_NEW,
			    _("New category"), _("Tap here to add a new category."),
			    G_CALLBACK (new_category_box), clist, -1);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_DELETE,
			    _("Delete category"), _("Tap here to delete the selected category."),
			    G_CALLBACK (ui_del_category), clist, -1);

  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), clist, TRUE, TRUE, 0);

  gtk_window_set_default_size (GTK_WINDOW (window), 240, 320);

  gtk_widget_show_all (window);
}
