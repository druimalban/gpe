/*
 * Copyright (C) 2001, 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#include "support.h"
#include "db.h"
#include "proto.h"
#include "categories.h"

#include <gpe/gtkdatecombo.h>

extern GtkWidget *clist;
extern gchar *active_chars;
extern void edit_person (struct person *p);
extern GtkWidget *mainw;
extern gboolean panel_config_has_changed;

static gint panel_active_row = -1;

void 
store_filename (GtkWidget * w, GtkFileSelection * selector)
{
  const gchar *selected_filename =
    gtk_file_selection_get_filename (GTK_FILE_SELECTION (selector));
}

void
on_edit_bt_image_clicked (GtkButton * button, gpointer user_data)
{
  GtkWidget *filesel = gtk_file_selection_new ("Select image");

  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		      "clicked", G_CALLBACK (store_filename), filesel);

  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		    "clicked", G_CALLBACK (gtk_widget_destroy),
		    (gpointer) filesel);

  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
		    "clicked", G_CALLBACK (gtk_widget_destroy),
		    (gpointer) filesel);

  gtk_widget_show_all (filesel);
}

void
on_edit_cancel_clicked (GtkButton * button, gpointer user_data)
{
  gtk_widget_destroy (GTK_WIDGET (user_data));
}

void
retrieve_special_fields (GtkWidget * edit, struct person *p)
{
  GSList *cl = g_object_get_data (G_OBJECT (edit), "category-widgets");
  db_delete_tag (p, "CATEGORY");
  while (cl)
    {
      GtkWidget *w = cl->data;
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
	{
	  guint c = (guint) g_object_get_data (G_OBJECT (w), "category");
	  char buf[32];
	  snprintf (buf, sizeof (buf) - 1, "%d", c);
	  buf[sizeof (buf) - 1] = 0;
	  db_set_multi_data (p, "CATEGORY", g_strdup (buf));
	}
      cl = cl->next;
    }

  db_delete_tag (p, "BIRTHDAY");
  {
    GtkDateCombo *c = GTK_DATE_COMBO (lookup_widget (edit, "datecombo"));
    if (c->set)
      {
	char buf[32];
	snprintf (buf, sizeof (buf) - 1, "%04d%02d%02d", c->year, c->month,
		  c->day);
	buf[sizeof (buf) - 1] = 0;
	db_set_data (p, "BIRTHDAY", g_strdup (buf));
      }
  }
}

void
on_edit_save_clicked (GtkButton * button, gpointer user_data)
{
  GtkWidget *edit = (GtkWidget *) user_data;
  GSList *tags;
  struct person *p = g_object_get_data (G_OBJECT (edit), "person");
  if (p == NULL)
    p = new_person ();
  
  for (tags = g_object_get_data (G_OBJECT (edit), "tag-widgets");
       tags; tags = tags->next)
    {
      GtkWidget *w = tags->data;
      gchar *text, *tag;
      if (GTK_IS_EDITABLE (w))
	text = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
      else
	{
	  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w));
	  GtkTextIter start, end;
	  gtk_text_buffer_get_bounds (buf, &start, &end);
	  text = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
	}
      tag = g_object_get_data (G_OBJECT (w), "db-tag");
      db_set_data (p, tag, text);
    }

  retrieve_special_fields (edit, p);

  if (commit_person (p))
    {
      gtk_widget_destroy (edit);
      discard_person (p);
      update_display ();
    }
}

void
on_categories_clicked (GtkButton *button, gpointer user_data)
{
  struct person *p;

  p = g_object_get_data (G_OBJECT (user_data), "person");

  change_categories (p);
}

// configuration 

void
on_bDetAdd_clicked (GtkButton * button, gpointer user_data)
{
  GtkWidget *clist;
  GtkWidget *entry;
  GtkWidget *cbox;
  gchar *strvec[2];

  clist = lookup_widget (mainw, "clist8");
  entry = lookup_widget (mainw, "entry2");
  cbox = lookup_widget (mainw, "cbField");
  strvec[1] = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

  if (strlen (strvec[1]) > 0)
    {
      strvec[0] =
	gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO (cbox)->entry), 0,
				-1);
      gtk_clist_append (GTK_CLIST (clist), strvec);
      gtk_entry_set_text (GTK_ENTRY (entry), "");
      db_add_config_values (CONFIG_PANEL, strvec[1], strvec[0]);
      g_free (strvec[0]);
      g_free (strvec[1]);
      panel_config_has_changed = TRUE;
    }
}


void
on_bDetRemove_clicked (GtkButton * button, gpointer user_data)
{
  GtkWidget *clist;
  gchar *identifier;
  if (panel_active_row > 0)
    {
      clist = lookup_widget (mainw, "clist8");
      gtk_clist_get_text (GTK_CLIST (clist), panel_active_row, 1,
			  &identifier);
      db_delete_config_values (CONFIG_PANEL, identifier);
      gtk_clist_remove (GTK_CLIST (clist), panel_active_row);
      panel_config_has_changed = TRUE;
    }
}


void
on_clist8_select_row (GtkCList * clist,
		      gint row,
		      gint column, GdkEvent * event, gpointer user_data)
{
  panel_active_row = row;
}


void
on_clist8_unselect_row (GtkCList * clist,
			gint row,
			gint column, GdkEvent * event, gpointer user_data)
{
  panel_active_row = -1;
}

void
on_setup_destroy (GtkObject * object, gpointer user_data)
{
  if (panel_config_has_changed)
    load_panel_config ();
}
