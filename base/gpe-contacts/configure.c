/*
 * Copyright (C) 2001, 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

/*
 * $Id$
 *
 */

#include <gtk/gtk.h>
#include <libintl.h>
#include <stdlib.h>
#include <string.h>
#include <libdisplaymigration/displaymigration.h>
#include <unistd.h>
#include <locale.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/smallbox.h>
#include <gpe/errorbox.h>
#include <gpe/gtkdatecombo.h>
#include <gpe/question.h>
#include <gpe/gtksimplemenu.h>
#include <gpe/picturebutton.h>
#include <gpe/spacing.h>

#include "support.h"
#include "db.h"
#include "structure.h"
#include "proto.h"
#include "main.h"

extern GtkWidget *mainw;

void
on_bDetAdd_clicked (GtkButton * button, GtkWidget *tree_view)
{
  GtkWidget *entry;
  GtkWidget *cbox;
  gchar *label;
  GtkListStore *list_store;

  list_store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view)));

  entry = lookup_widget (mainw, "entry2");
  cbox = lookup_widget (mainw, "cbField");
  label = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

  if (strlen (label) > 0)
    {
      GtkTreeIter iter;
      gchar *tag;

      tag = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO (cbox)->entry), 0, -1);

      gtk_entry_set_text (GTK_ENTRY (entry), "");

      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter, 0, label, 1, tag, -1);
    }
  else
    g_free (label);
}

void
on_bDetRemove_clicked (GtkButton * button, GtkWidget *tree_view)
{
  GtkTreeSelection *sel;
  GList *list, *iter;
  GSList *refs = NULL, *riter;
  GtkTreeModel *model;

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));

  list = gtk_tree_selection_get_selected_rows (sel, &model);

  for (iter = list; iter; iter = iter->next)
    {
      GtkTreePath *path;
      GtkTreeRowReference *ref;

      path = list->data;
      ref = gtk_tree_row_reference_new (model, path);
      gtk_tree_path_free (path);

      refs = g_slist_prepend (refs, ref);
    }

  g_list_free (list);

  for (riter = refs; riter; riter = riter->next)
    {
      GtkTreeRowReference *ref;
      GtkTreePath *path;

      ref = riter->data;
      path = gtk_tree_row_reference_get_path (ref);
      if (path)
	{
	  GtkTreeIter it;
	  
	  if (gtk_tree_model_get_iter (model, &it, path))
	    gtk_list_store_remove (GTK_LIST_STORE (model), &it);

	  gtk_tree_path_free (path);
	}

      gtk_tree_row_reference_free (ref);
    }

  g_slist_free (refs);
}

void
on_compact_clicked (GtkButton *button, GtkWidget *lsize)
{
  gchar *str;
  str = db_compress();
  if (str) 
    {
      fprintf(stderr, "err %s\n", str);
    }
  else
    {
      str = g_strdup_printf("%s %d %s", _("Current size:"), db_size(), 
                            _("kB (optimised)"));
      gtk_label_set_text(GTK_LABEL(lsize), str);
    }
  g_free(str);
}

/* This create the setup for portrait displays to configure the details panel
   fields and order */
GtkWidget *
create_pageSetup (GObject *container)
{
  GtkWidget *wSetup;
  GtkWidget *vbox9;
  GtkWidget *frame2;
  GtkWidget *vbox10;
  GtkWidget *hbox4;
  GtkWidget *cbField;
  GList *cbField_items = NULL;
  GtkWidget *combo_entry1;
  GtkWidget *entry2;
  GtkWidget *bDetAdd;
  GtkWidget *bDetRemove;
  GtkWidget *scrolledwindow1;
  gint tagcount, i;
  gchar **taglist;
  GtkListStore *list_store;
  GtkWidget *tree_view;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;
  gchar *s;

  wSetup = mainw;

  list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
  g_object_set_data (container, "list-store", list_store);

  tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));

  /*
     glade generated stuff starts here
     this is the container, child in tab page     
   */
  vbox9 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox9);

  frame2 = gtk_label_new (NULL);
  s = g_strdup_printf("<b>%s</b>", _("Detail panel:"));
  gtk_label_set_markup (GTK_LABEL (frame2), s);
  g_free (s);

  gtk_misc_set_alignment (GTK_MISC (frame2), 0.0, 0.5);
  gtk_widget_show (frame2);
  gtk_box_pack_start (GTK_BOX (vbox9), frame2, FALSE, FALSE, 0);

  vbox10 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox10);
  gtk_box_pack_start (GTK_BOX (vbox9), vbox10, TRUE, TRUE, 0);

  hbox4 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox4);

  cbField = gtk_combo_new ();
  gtk_widget_set_name (cbField, "cbField");
  gtk_widget_ref (cbField);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "cbField", cbField,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (cbField);
  gtk_box_pack_start (GTK_BOX (hbox4), cbField, TRUE, TRUE, 0);

  combo_entry1 = GTK_COMBO (cbField)->entry;
  gtk_widget_set_name (combo_entry1, "combo_entry1");
  gtk_widget_ref (combo_entry1);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "combo_entry1", combo_entry1,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (combo_entry1);
  gtk_entry_set_text (GTK_ENTRY (combo_entry1), _("NAME"));

  entry2 = gtk_entry_new ();
  gtk_widget_set_name (entry2, "entry2");
  gtk_widget_ref (entry2);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "entry2", entry2,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (entry2);
  gtk_box_pack_start (GTK_BOX (hbox4), entry2, TRUE, TRUE, 0);

  bDetAdd = gtk_button_new_with_label (_("Add"));
  gtk_widget_show (bDetAdd);
  gtk_box_pack_start (GTK_BOX (hbox4), bDetAdd, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (bDetAdd), "clicked",
		    G_CALLBACK (on_bDetAdd_clicked), tree_view);

  bDetRemove = gtk_button_new_with_label (_("Remove"));
  gtk_widget_show (bDetRemove);
  gtk_box_pack_start (GTK_BOX (hbox4), bDetRemove, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (bDetRemove), "clicked",
		    G_CALLBACK (on_bDetRemove_clicked), tree_view);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (vbox10), scrolledwindow1, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (scrolledwindow1), tree_view);
  gtk_widget_show (tree_view);

  gtk_box_pack_start (GTK_BOX (vbox10), hbox4, FALSE, TRUE, 0);

  /* handcoded stuff */

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
  col = gtk_tree_view_column_new_with_attributes (_("Label"), renderer,
						  "text", 0,
						  NULL);

  gtk_tree_view_insert_column (GTK_TREE_VIEW (tree_view), col, -1);
  col = gtk_tree_view_column_new_with_attributes (_("Data item"), renderer,
						  "text", 1,
						  NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (tree_view), col, -1);

  tagcount = db_get_tag_list (&taglist);
  for (i = 0; i < tagcount; i++)
    {
      cbField_items =
	g_list_append (cbField_items, (gpointer) taglist[i + 1]);
    }
  gtk_combo_set_popdown_strings (GTK_COMBO (cbField), cbField_items);
  g_list_free (cbField_items);
  db_free_result (taglist);

  // reused some variables here...
  tagcount = db_get_config_values (CONFIG_PANEL, &taglist);
  for (i = 1; i <= tagcount; i++)
    {
      /* label, then tag */
      GtkTreeIter iter;

      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter, 0, taglist[2 * i], 1, taglist[2 * i + 1], -1);
    }

  db_free_result (taglist);
  return vbox9;
}

/* This one is for lanscape diplays creating setup for position of list
   and related stuff */

GtkWidget *
create_pageLandSetup (GObject *container)
{
  GtkWidget *table = gtk_table_new(3, 3, FALSE);
  GtkWidget *label, *w;
  gchar *markup, *cfgval;

  gtk_container_set_border_width(GTK_CONTAINER(table), gpe_get_border());
  gtk_table_set_col_spacings(GTK_TABLE(table), gpe_get_boxspacing());
  gtk_table_set_row_spacings(GTK_TABLE(table), gpe_get_boxspacing());
    
  markup = g_strdup_printf("<b>%s</b>", _("Screen Layout"));
  label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(label), markup);
  g_free(markup);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 3, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  label = gtk_label_new(_("List position"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  w = gtk_radio_button_new_with_label(NULL, _("left"));
  gtk_table_attach(GTK_TABLE(table), w, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  w = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(w), _("right"));
  gtk_table_attach(GTK_TABLE(table), w, 2, 3, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  cfgval = db_get_config_tag (CONFIG_LIST, "pos");
  if (cfgval)
    {
      if (!strcmp(cfgval, "right"))
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), FALSE);
      g_free(cfgval);
    }
  g_object_set_data (container, "rb-right", w);
  return table;
}

/* This page contains database related controls. */

GtkWidget *
create_pageDatabase (GObject *container)
{
  GtkWidget *table = gtk_table_new(3, 3, FALSE);
  GtkWidget *label, *w, *lsize;
  gchar *markup;

  gtk_container_set_border_width(GTK_CONTAINER(table), gpe_get_border());
  gtk_table_set_col_spacings(GTK_TABLE(table), gpe_get_boxspacing());
  gtk_table_set_row_spacings(GTK_TABLE(table), gpe_get_boxspacing());
    
  markup = g_strdup_printf("<b>%s</b>", _("Database Size"));
  label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(label), markup);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  g_free(markup);
  gtk_table_attach(GTK_TABLE(table), label, 0, 3, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  /* TRANSLATORS: kB = kilo bytes */
  markup = g_strdup_printf("%s %d %s", _("Current size:"), db_size(), _("kB"));
  lsize = gtk_label_new(markup);
  gtk_misc_set_alignment(GTK_MISC(lsize), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), lsize, 0, 3, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  g_free(markup);
  markup = g_strdup_printf("<b>%s</b>", _("Actions"));
  label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(label), markup);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  g_free(markup);
  gtk_table_attach(GTK_TABLE(table), label, 0, 3, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  
  label = gtk_label_new(_("Optimise database"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
  w = gtk_button_new_with_label(_("Run"));
  gtk_table_attach(GTK_TABLE(table), w, 1, 3, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
  g_signal_connect(G_OBJECT(w), "clicked", G_CALLBACK(on_compact_clicked), lsize);
  
  return table;
}

static void
configure_ok_clicked (GtkWidget *widget, GtkWidget *window)
{
  GtkListStore *list_store;
  GtkTreeIter iter;
  gchar **taglist;
  GSList *list = NULL;
  int tagcount;
  int i;
  GtkWidget *w, *wh, *wl;
	  
  list_store = g_object_get_data (G_OBJECT (window), "list-store");

  if (mode_landscape)
    {
      w = g_object_get_data (G_OBJECT (window), "rb-right");
      wh = g_object_get_data (G_OBJECT (mainw), "hbox");
      wl = g_object_get_data (G_OBJECT (mainw), "swlist");
    
      if (w && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
        {
          db_update_config_values(CONFIG_LIST, "pos", "right");
          if (wh && wl)
            gtk_box_set_child_packing(GTK_BOX(wh), wl, FALSE, TRUE, 0, GTK_PACK_END);
        }
      else
        {
          db_update_config_values(CONFIG_LIST, "pos", "left");
          if (wh && wl)
            gtk_box_set_child_packing(GTK_BOX(wh), wl, FALSE, TRUE, 0, GTK_PACK_START);
        }
    }
  /* panel config */
  tagcount = db_get_config_values (CONFIG_PANEL, &taglist);
  
  if (list_store)
    {
      if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store), &iter))
        {
          do 
        {
          gboolean found = FALSE;
          gchar *tag, *label;
    
          gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 0, &label, 1, &tag, -1);
    
          for (i = 1; i <= tagcount; i++)
            {
              gchar *ttag = taglist[2 * i];
              
              if (!strcasecmp (label, ttag))
            {
              found = TRUE;
              break;
            }
            }
    
          list = g_slist_prepend (list, label);
    
          if (!found)
            db_add_config_values (CONFIG_PANEL, label, tag);
    
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (list_store), &iter));
        }
    
      for (i = 1; i <= tagcount; i++)
        {
          gboolean found = FALSE;
          gchar *ttag = taglist[2 * i];
          GSList *iter;
    
          for (iter = list; iter; iter = iter->next)
        {
          if (!strcasecmp (ttag, iter->data))
            {
              found = TRUE;
              break;
            }
        }
    
          if (!found)
        db_delete_config_values (CONFIG_PANEL, ttag);
        }
      
      g_slist_free (list);
    
      db_free_result (taglist);
    
      load_panel_config ();
    }
  gtk_widget_destroy (window);
}

void
configure (GtkWidget * widget, gpointer d)
{
  gboolean enable_structure = (gboolean) d;
  GtkWidget *window = gtk_dialog_new ();
  GtkWidget *notebook = gtk_notebook_new ();
  GtkWidget *editlabel = gtk_label_new (_("Editing Layout"));
  GtkWidget *editbox = edit_structure ();
  GtkWidget *configlabel = gtk_label_new (_("Setup"));
  GtkWidget *dblabel = gtk_label_new (_("Database"));
  GtkWidget *configbox, *dbbox;
  GtkWidget *ok_button, *cancel_button;

  if (mode_landscape)
    configbox = create_pageLandSetup (G_OBJECT (window));
  else  
    configbox = create_pageSetup (G_OBJECT (window));
  
  dbbox = create_pageDatabase (G_OBJECT (window));
  
  if (enable_structure)
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), editbox, editlabel);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), configbox, configlabel);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), dbbox, dblabel);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->vbox), notebook);

  ok_button = gtk_button_new_from_stock (GTK_STOCK_OK);
  cancel_button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), cancel_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), ok_button, FALSE, FALSE, 0);

  gtk_window_set_default_size (GTK_WINDOW (window), 240, 300);

  g_signal_connect (G_OBJECT (ok_button), "clicked",
		    G_CALLBACK (configure_ok_clicked), window);

  g_signal_connect_swapped (G_OBJECT (cancel_button), "clicked",
			    G_CALLBACK (gtk_widget_destroy), window);

  gtk_window_set_title (GTK_WINDOW (window), _("Contacts: Configuration"));
  gpe_set_window_icon (window, "icon");

  displaymigration_mark_window (window);

  gtk_widget_show_all (window);
}
