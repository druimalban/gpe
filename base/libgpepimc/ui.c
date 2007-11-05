/*
 * Copyright (C) 2001, 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 *               2006 Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>
#ifndef IS_HILDON
#include <gpe/colordialog.h>
#endif
#include <gpe/picturebutton.h>

#include "gpe/pim-categories.h"

#include "internal.h"

#ifdef IS_HILDON
#if HILDON_VER > 0
#include <hildon/hildon-caption.h>
#include <hildon/hildon-color-button.h>
#else
#include <hildon-widgets/hildon-caption.h>
#include <hildon-widgets/hildon-color-button.h>
#endif
#endif

enum 
{ 
  LS_CHECKED,
  LS_NAME,
  LS_ID,
  LS_COLOR,
  LS_MAX
};

#define _(x) (x)

#ifndef IS_HILDON
static gchar *col_palette_red    = "#FF0000";
static gchar *col_palette_green  = "#00FF00";
static gchar *col_palette_yellow = "#FFFF00";
static gchar *col_palette_blue   = "#0000FF";
#endif

static void
set_widget_color_str (GtkWidget *widget, const gchar *colstr)
{
  GdkColormap *map;
  static GdkColor colour;
    
  gdk_color_parse (colstr, &colour);
  map = gdk_colormap_get_system ();
  gdk_colormap_alloc_color (map, &colour, FALSE, TRUE);
#ifdef IS_HILDON
  hildon_color_button_set_color (HILDON_COLOR_BUTTON (widget), &colour);
#else
  gtk_widget_modify_bg (widget, GTK_STATE_NORMAL, &colour);
  gtk_widget_modify_fg (widget, GTK_STATE_NORMAL, &colour);
#endif
}

#ifdef IS_HILDON
static void
color_changed (HildonColorButton *color_button, GParamSpec *spec, gpointer data)
{
  GtkWidget *parent = data;
  GdkColor *color;
  gchar color_str[8];

#if HILDON_VER > 0
  hildon_color_button_get_color (color_button, &color);
#else
  color = hildon_color_button_get_color (color_button);
#endif
  g_snprintf (color_str, 8, "#%02x%02x%02x", color->red / 257, color->green  / 257, color->blue / 257);
  gdk_color_free (color);

  g_object_set_data_full (G_OBJECT (parent), "col", g_strdup (color_str), g_free);
}
#else
static void
palette_color (GtkWidget *w, gpointer data)
{
  GtkWidget *parent = gtk_widget_get_toplevel (w);
  GtkWidget *preview = g_object_get_data (G_OBJECT (parent), "preview");
  gchar *cur_col = g_object_get_data (G_OBJECT (parent), "col");
  const gchar *colstr = data;
    
  g_object_set_data_full (G_OBJECT (parent), "col", g_strdup (colstr), g_free);
  set_widget_color_str (preview, colstr);
}

static void
select_color (GtkWidget *w, gpointer data)
{
  GtkWidget *parent = data;
  GtkWidget *dlg;
  GtkWidget *preview = g_object_get_data (G_OBJECT (parent), "preview");
  gchar *cur_col = g_object_get_data (G_OBJECT (parent), "col");
 
  dlg = gpe_color_dialog_new (GTK_WINDOW(parent), GTK_DIALOG_MODAL, 
                              cur_col ? cur_col : "#FFFFFF");
  
  if (gtk_dialog_run (GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT)
    {
      const gchar *col = gpe_color_dialog_get_color_str(GPE_COLOR_DIALOG (dlg));
      g_object_set_data_full (G_OBJECT (parent), "col", g_strdup (col), g_free);
      set_widget_color_str (preview, col);      
    }
  gtk_widget_destroy (dlg);    
}
#endif

static void
do_update_category (GtkWidget *widget, GtkWidget *d)
{
  GtkWidget *entry = g_object_get_data (G_OBJECT (d), "entry");
  gchar *sel_title = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
  gchar *col = g_object_get_data (G_OBJECT (d), "col"); 
  GtkTreeIter iter;
  GtkListStore *list_store;
  GtkTreePath *sel_path = (GtkTreePath *) g_object_get_data (G_OBJECT (d), "path"); /* will be NULL if new */

  list_store = g_object_get_data (G_OBJECT (d), "list-store");

  if (sel_title[0] == 0)
    {
      gpe_error_box (_("Category name must not be blank"));
      gtk_widget_destroy (d);
      return;
    }

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store), &iter))
    {
      do
        {
          gchar *title;

          gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, LS_NAME, &title, -1);
          if (!strcasecmp (title, sel_title))
            {
              GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (list_store), &iter);
              if (!sel_path || gtk_tree_path_compare (path, sel_path))
                {
                  gtk_tree_path_free (path);
                  g_free (title);
                  gpe_error_box (_("A category by that name already exists"));
                  gtk_widget_destroy (d);
                  return;
                }
              gtk_tree_path_free (path);
            }
          g_free (title);
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (list_store), &iter));
    }

  if (sel_path == NULL)
    {
      gtk_list_store_append (list_store, &iter);

      gtk_list_store_set (list_store, &iter, LS_CHECKED, FALSE,
                      LS_NAME, sel_title, LS_ID, -1, LS_COLOR, col, -1);
    }
  else
    {
      gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), &iter, sel_path);
 
      gtk_list_store_set (list_store, &iter, LS_NAME, sel_title, LS_COLOR, col, -1);
   }

  gtk_widget_destroy (d);
}

static void
category_dialog (GtkWidget *w, GtkTreeView *tree_view, gboolean new)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *ok;
  GtkWidget *cancel;
#ifndef IS_HILDON
  GtkWidget *label;
#endif
  GtkWidget *name;
#ifdef IS_HILDON
  GtkSizeGroup *group;
  GtkWidget *caption;
#else
  GtkWidget *table;
  GtkWidget *clabel;
#endif
  GtkWidget *cbutton;
#ifndef IS_HILDON
  GtkWidget *previewbox, *previewbutton;
  GtkWidget *hbox;
  GtkWidget *redbutton, *greenbutton, *yellowbutton, *bluebutton;
  GtkWidget *redbox, *greenbox, *yellowbox, *bluebox;
#endif
  guint spacing;
  gint width, height;
  GtkListStore *list_store = GTK_LIST_STORE (gtk_tree_view_get_model (tree_view));

  spacing = gpe_get_boxspacing ();
  gtk_icon_size_lookup (GTK_ICON_SIZE_SMALL_TOOLBAR, &width, &height);

  window = gtk_dialog_new ();

  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (gtk_widget_get_toplevel (w)));

  vbox = GTK_DIALOG (window)->vbox;

#ifdef IS_HILDON
  group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  name = gtk_entry_new ();
  caption = hildon_caption_new (group, _("Name"), name, NULL, HILDON_CAPTION_OPTIONAL);
  gtk_box_pack_start (GTK_BOX (vbox), caption, FALSE, FALSE, spacing);
  
  cbutton = hildon_color_button_new ();
  caption = hildon_caption_new (group, _("Color"), cbutton, NULL, HILDON_CAPTION_OPTIONAL);
  gtk_box_pack_start (GTK_BOX (vbox), caption, FALSE, FALSE, spacing);

  g_signal_connect (G_OBJECT (cbutton), "notify::color", G_CALLBACK (color_changed), window); 
#else
  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), spacing);
  gtk_table_set_row_spacings (GTK_TABLE (table), spacing);

  label = gtk_label_new (_("Name:"));
  name = gtk_entry_new ();
  clabel = gtk_label_new (_("Colour:"));

  hbox = gtk_hbox_new (FALSE, spacing);

  redbutton    = gtk_button_new ();
  greenbutton  = gtk_button_new ();
  yellowbutton = gtk_button_new ();
  bluebutton   = gtk_button_new ();
  previewbutton= gtk_button_new ();
  cbutton = gpe_button_new_from_stock (GTK_STOCK_SELECT_COLOR, GPE_BUTTON_TYPE_ICON);
  gtk_button_set_relief (GTK_BUTTON (previewbutton), GTK_RELIEF_NONE);

  redbox    = gtk_event_box_new ();
  greenbox  = gtk_event_box_new ();
  yellowbox = gtk_event_box_new ();
  bluebox   = gtk_event_box_new ();
  previewbox= gtk_event_box_new ();

  gtk_widget_set_size_request (redbox,    width, height);
  gtk_widget_set_size_request (greenbox,  width, height);
  gtk_widget_set_size_request (yellowbox, width, height);
  gtk_widget_set_size_request (bluebox,   width, height);
  gtk_widget_set_size_request (previewbox, width, height);
  
  gtk_container_add (GTK_CONTAINER (redbutton),    redbox);
  gtk_container_add (GTK_CONTAINER (greenbutton),  greenbox);
  gtk_container_add (GTK_CONTAINER (yellowbutton), yellowbox);
  gtk_container_add (GTK_CONTAINER (bluebutton),   bluebox);
  gtk_container_add (GTK_CONTAINER (previewbutton),previewbox);

  gtk_box_pack_start (GTK_BOX (hbox), redbutton, FALSE, TRUE, 0); 
  gtk_box_pack_start (GTK_BOX (hbox), greenbutton, FALSE, TRUE, 0); 
  gtk_box_pack_start (GTK_BOX (hbox), yellowbutton, FALSE, TRUE, 0); 
  gtk_box_pack_start (GTK_BOX (hbox), bluebutton, FALSE, TRUE, 0); 
  gtk_box_pack_start (GTK_BOX (hbox), cbutton, FALSE, TRUE, 0);
  
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), name, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), clabel, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), previewbutton, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  
  g_signal_connect (G_OBJECT (cbutton), "clicked", G_CALLBACK (select_color), window);
  
  g_signal_connect (G_OBJECT (redbutton),   "clicked", G_CALLBACK (palette_color), col_palette_red);
  g_signal_connect (G_OBJECT (greenbutton), "clicked", G_CALLBACK (palette_color), col_palette_green);
  g_signal_connect (G_OBJECT (yellowbutton),"clicked", G_CALLBACK (palette_color), col_palette_yellow);
  g_signal_connect (G_OBJECT (bluebutton),  "clicked", G_CALLBACK (palette_color), col_palette_blue);

  set_widget_color_str (redbox,    col_palette_red);
  set_widget_color_str (greenbox,  col_palette_green);
  set_widget_color_str (yellowbox, col_palette_yellow);
  set_widget_color_str (bluebox,   col_palette_blue);
#endif

#ifdef IS_HILDON
  ok = gtk_button_new_with_label (_("OK"));
  cancel = gtk_button_new_with_label (_("Cancel"));

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), ok, 
		      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), cancel, 
		      FALSE, FALSE, 0);
#else
  ok = gtk_button_new_from_stock (GTK_STOCK_OK);
  cancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), cancel, 
		      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), ok, 
		      FALSE, FALSE, 0);
#endif
  GTK_WIDGET_SET_FLAGS (ok, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (ok);

  if (new)  
    {
      gtk_window_set_title (GTK_WINDOW (window), _("New category"));
      g_object_set_data (G_OBJECT (window), "path", NULL);
#ifdef IS_HILDON
      set_widget_color_str (cbutton, "white");
#else
      set_widget_color_str (previewbox, "white");
#endif
    }
  else
    {
      GtkTreeSelection *sel;
      GtkTreeIter cur_iter;
      GtkTreeModel *model;
      GtkTreePath *selected_path;
      gchar *cur_name, *cur_colour;
        
      gtk_window_set_title (GTK_WINDOW (window), _("Edit category"));

      sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
      gtk_tree_selection_get_selected (sel, &model, &cur_iter);
      selected_path = gtk_tree_model_get_path (model, &cur_iter);
      gtk_tree_model_get (GTK_TREE_MODEL (list_store), &cur_iter, LS_NAME, &cur_name, 
                          LS_COLOR, &cur_colour, -1);
      if (cur_name)
          gtk_entry_set_text (GTK_ENTRY (name), cur_name);
      g_free (cur_name);

      g_object_set_data_full (G_OBJECT (window), "col", cur_colour, g_free);
      g_object_set_data_full (G_OBJECT (window), "path", selected_path, 
                              (GDestroyNotify) gtk_tree_path_free);
      
      if (cur_colour) 
#ifdef IS_HILDON
       set_widget_color_str (cbutton, cur_colour);
#else
       set_widget_color_str (previewbox, cur_colour);
#endif
    }

  g_signal_connect (G_OBJECT (ok), "clicked", 
                    G_CALLBACK (do_update_category), window);
  g_signal_connect (G_OBJECT (name), "activate", 
                    G_CALLBACK (do_update_category), window);
  g_signal_connect_swapped (G_OBJECT (cancel), "clicked", 
			    G_CALLBACK (gtk_widget_destroy), window);

#ifndef IS_HILDON
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, spacing);
#endif

  g_object_set_data (G_OBJECT (window), "entry", name);
#ifndef IS_HILDON
  g_object_set_data (G_OBJECT (window), "preview", previewbox);
#endif
 
  gpe_set_window_icon (window, "icon");

  gtk_container_set_border_width (GTK_CONTAINER (window), gpe_get_border ());


  g_object_set_data (G_OBJECT (window), "list-store", list_store);

  gtk_widget_show_all (window);
  gtk_widget_grab_focus (name);
}

static void
new_category (GtkWidget *w, GtkTreeView *tree_view)
{
  category_dialog (w, tree_view, TRUE);
}

static void
modify_category (GtkWidget *w, GtkTreeView *tree_view)
{
  category_dialog (w, tree_view, FALSE);
}

static void
delete_category (GtkWidget *w, GtkWidget *tree_view)
{
  GtkTreeSelection *sel;
  GList *list, *iter;
  GtkTreeModel *model;
  GSList *refs = NULL, *riter;

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

static void
category_toggled(GtkCellRendererToggle *renderer, gchar *path, gpointer data)
{
  GtkTreePath *tpath = gtk_tree_path_new_from_string(path);
  GtkTreeIter iter;
  GtkListStore *list_store = data;
  gboolean active;

  gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), &iter, tpath);
  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, LS_CHECKED, &active, -1);
  active = !active;
  gtk_list_store_set (GTK_LIST_STORE (list_store), &iter, LS_CHECKED, active, -1);
  gtk_tree_path_free(tpath);
}

static void
categories_dialog_cancel (GtkWidget *w, gpointer p)
{
  GtkWidget *window = GTK_WIDGET (p);

  gtk_widget_destroy (window);
}

static void
categories_dialog_ok (GtkWidget *w, gpointer p)
{
  GtkWidget *window;
  GtkTreeIter iter;
  GtkListStore *list_store;
  GSList *old_categories, *i;
  GSList *selected_categories = NULL;
  void (*callback) (GtkWidget *, GSList *, gpointer);
  gpointer data;

  window = GTK_WIDGET (p);
  list_store = g_object_get_data (G_OBJECT (window), "list_store");

  old_categories = gpe_pim_categories_list ();

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store), &iter))
    {
      do 
	{
	  gint id;
	  gchar *title, *col;
	  gboolean selected;
	  
	  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 
                          LS_CHECKED, &selected, LS_NAME, &title, 
                          LS_ID, &id, LS_COLOR, &col, -1);

	  if (id == -1)
	    {
              GSList *new_categories;
	      struct gpe_pim_category *c = NULL;

	      gpe_pim_category_new (title, &id);
	      gpe_pim_category_set_colour (id, col);

              new_categories = gpe_pim_categories_list ();
              for (i = new_categories; i; i = i->next)
                {
                  c = i->data;

                  if (c->id == id)
                    break;
                }
              g_slist_free (new_categories);

              if (i)
                c->colour = g_strdup (col);
	    }
	  else
	    {
	      struct gpe_pim_category *c = NULL;

	      for (i = old_categories; i; i = i->next)
            {
              c = i->data;
              
              if (c->id == id)
                break;
            }

	      if (i)
            {
              old_categories = g_slist_remove_link (old_categories, i);
              g_slist_free (i);
              
              if (strcmp (c->name, title))
                {
                  g_free ((void *)c->name);
                  c->name = g_strdup (title);
                  gpe_pim_category_rename (id, title);
                }
              if ((!c->colour) || (col && (strcmp (c->colour, col))))
                {
                  if (c->colour) 
                    g_free ((void *)c->colour);
                  c->colour = g_strdup (col);
                  gpe_pim_category_set_colour (id, col);
                }
            }
	      else
            selected = FALSE;		/* category was deleted by second party */
	    }
	  if (selected)
	    selected_categories = g_slist_prepend (selected_categories, (gpointer)id);

          g_free (title);
          g_free (col);
	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (list_store), &iter));
    }

  for (i = old_categories; i; i = i->next)
    {
      struct gpe_pim_category *c = i->data;

      gpe_pim_category_delete (c);
    }

  g_slist_free (old_categories);

  callback = g_object_get_data (G_OBJECT (window), "callback");
  data = g_object_get_data (G_OBJECT (window), "callback-data");

  if (callback)
    (*callback) (window, selected_categories, data);
  g_slist_free (selected_categories);

  gtk_widget_destroy (window);
}

static void
change_category_name (GtkCellRendererText *cell,
                      gchar               *path_string,
                      gchar               *new_text,
                      gpointer             user_data)
{
  GtkListStore *list_store;
  GtkTreeIter iter;
  gint id;

  list_store = GTK_LIST_STORE (user_data);

  if (!gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (list_store),
                                            &iter, path_string))
    {
      gpe_error_box ("Error getting changed item");
      return;
    }

  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, LS_ID, &id, -1);

  /* Update the name in the list, it will be updated in the db on ok. */
  gtk_list_store_set (list_store, &iter, LS_NAME, new_text, -1);
}

static void 
list_select_row (GtkTreeView *treeview, gpointer data)
{
    GtkWidget *parent = data;
    GtkWidget *editbutton = g_object_get_data (G_OBJECT (parent), "edit-button");
    GtkWidget *deletebutton = g_object_get_data (G_OBJECT (parent), "delete-button");
    GtkTreeSelection *sel;
    
    sel = gtk_tree_view_get_selection(treeview);
    if (gtk_tree_selection_get_selected(sel, NULL, NULL))
      {
        if (editbutton)
          gtk_widget_set_sensitive(GTK_WIDGET(editbutton), TRUE);
        if (deletebutton)
          gtk_widget_set_sensitive(GTK_WIDGET(deletebutton), TRUE);
	  }
    else
      {
        if (editbutton)
          gtk_widget_set_sensitive(GTK_WIDGET(editbutton), FALSE);
        if (deletebutton)
          gtk_widget_set_sensitive(GTK_WIDGET(deletebutton), FALSE);
	  }
}

#ifdef IS_HILDON
GtkWidget *
gpe_pim_categories_dialog (GSList *selected_categories, gboolean select, 
                           GCallback callback, gpointer data)
#else
GtkWidget *
gpe_pim_categories_dialog (GSList *selected_categories, GCallback callback, gpointer data)
#endif
{
#ifndef IS_HILDON
  GtkWidget *toolbar;
#endif
  GtkWidget *window;
  GtkWidget *sw, *editbutton = NULL, *deletebutton = NULL;
  GSList *iter;
  GtkWidget *okbutton = NULL, *cancelbutton = NULL;
  GtkListStore *list_store;
  GtkWidget *tree_view;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;
  GSList *list;
  GtkTreeSelection *sel;

#ifdef IS_HILDON
  GtkWidget *newbutton = NULL;
#endif
    
  window = gtk_dialog_new ();

  list_store = gtk_list_store_new (LS_MAX, G_TYPE_BOOLEAN, G_TYPE_STRING, 
                                   G_TYPE_INT, G_TYPE_STRING);

  tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
  gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);

#ifdef IS_HILDON

    g_object_set(G_OBJECT(tree_view), "allow-checkbox-mode", FALSE, NULL);

#else
    toolbar = gtk_toolbar_new ();
    gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
    
    gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_NEW,
                  _("New category"), 
                  _("Tap here to add a new category."),
                  G_CALLBACK (new_category), tree_view, -1);
    
    editbutton = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_PROPERTIES,
                  _("Modify category"), 
                  _("Tap here to modify the selected category."),
                  G_CALLBACK (modify_category), tree_view, -1);
                  
    deletebutton = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_DELETE,
                  _("Delete category"), 
                  _("Tap here to delete the selected category."),
                  G_CALLBACK (delete_category), tree_view, -1);
    gtk_widget_set_sensitive (editbutton, FALSE);
    gtk_widget_set_sensitive (deletebutton, FALSE);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox),
                toolbar, FALSE, FALSE, 0);                
#endif      
  sw = gtk_scrolled_window_new (NULL, NULL);

  list = gpe_pim_categories_list ();
  for (iter = list; iter != NULL; iter = iter->next)
    {
      struct gpe_pim_category *c = iter->data;
      GtkTreeIter titer;

      gtk_list_store_append (list_store, &titer);
      gtk_list_store_set (list_store, &titer, 
			  LS_CHECKED, g_slist_find (selected_categories, (gpointer)c->id) ? TRUE : FALSE,
			  LS_NAME, c->name, 
			  LS_ID, c->id,
              LS_COLOR, c->colour, -1);
    }
  g_slist_free (list);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), "text", "  ", NULL);
  col = gtk_tree_view_column_new_with_attributes (NULL, renderer,
                              "cell-background", LS_COLOR, NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (tree_view), col, -1);
#ifdef IS_HILDON
  if (select)
#endif
    {  
      renderer = gtk_cell_renderer_toggle_new ();
      g_object_set (G_OBJECT (renderer), "activatable", TRUE, NULL);
      col = gtk_tree_view_column_new_with_attributes (NULL, renderer,
                              "active", LS_CHECKED, NULL);
    
      gtk_tree_view_insert_column (GTK_TREE_VIEW (tree_view), col, -1);
    
      g_object_set_data (G_OBJECT (tree_view), "toggle-col", col);
      g_signal_connect(G_OBJECT(renderer), "toggled", 
                       G_CALLBACK(category_toggled), (gpointer) list_store);
    }
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
  col = gtk_tree_view_column_new_with_attributes (NULL, renderer, 
                                                  "text", LS_NAME, NULL);

  g_signal_connect (G_OBJECT (renderer), "edited", 
                    G_CALLBACK(change_category_name),
                    list_store);

  gtk_tree_view_insert_column (GTK_TREE_VIEW (tree_view), col, -1);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree_view), FALSE);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				  GTK_POLICY_NEVER,
				  GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (sw), tree_view);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), sw, TRUE, TRUE, 0);

  gpe_set_window_icon (window, "icon");

#ifdef IS_HILDON
  if (select)
  {
    okbutton = gtk_button_new_with_label (_("OK"));
    cancelbutton = gtk_button_new_with_label (_("Cancel"));
  }
  else
  {
    okbutton = gtk_button_new_with_label (_("Close"));
    newbutton = gtk_button_new_with_label (_("New"));
    editbutton = gtk_button_new_with_label (_("Edit"));
    deletebutton = gtk_button_new_with_label (_("Delete"));
    gtk_widget_set_sensitive (deletebutton, FALSE);
    gtk_widget_set_sensitive (editbutton, FALSE);
    g_signal_connect (G_OBJECT (editbutton), "clicked", 
                      G_CALLBACK (modify_category), tree_view);
  }
#else
  okbutton = gtk_button_new_from_stock (GTK_STOCK_OK);
  cancelbutton = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
#endif

  g_object_set_data (G_OBJECT (window), "list_store", list_store);
  g_object_set_data (G_OBJECT (window), "edit-button", editbutton);
  g_object_set_data (G_OBJECT (window), "delete-button", deletebutton);

  if (okbutton) 
    g_signal_connect (G_OBJECT (okbutton), "clicked", G_CALLBACK (categories_dialog_ok), window);
  if (cancelbutton)
    g_signal_connect (G_OBJECT (cancelbutton), "clicked", G_CALLBACK (categories_dialog_cancel), window);

#ifdef IS_HILDON
gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(tree_view), FALSE);
    
if (select)
  {
    gtk_window_set_title (GTK_WINDOW (window), _("Select categories"));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), okbutton, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), cancelbutton, TRUE, TRUE, 0);
  }
else
  {
    gtk_window_set_title (GTK_WINDOW (window), _("Edit categories"));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), newbutton, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), editbutton, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), deletebutton, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), okbutton, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (newbutton), "clicked", G_CALLBACK (new_category), tree_view);
    g_signal_connect (G_OBJECT (deletebutton), "clicked", G_CALLBACK (delete_category), tree_view);
  }
#else
  gtk_window_set_title (GTK_WINDOW (window), _("Select categories"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), cancelbutton, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), okbutton, TRUE, TRUE, 0);
#endif
  
  GTK_WIDGET_SET_FLAGS (okbutton, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (okbutton);
  
  gtk_window_set_default_size (GTK_WINDOW (window), 240, 320);

  g_signal_connect_swapped (G_OBJECT (window), "destroy", G_CALLBACK (g_object_unref), list_store);
  g_signal_connect_after(G_OBJECT(tree_view), "cursor-changed", 
	                     G_CALLBACK(list_select_row), (gpointer)(window)); 

  g_object_set_data (G_OBJECT (window), "callback", callback);
  g_object_set_data (G_OBJECT (window), "callback-data", data);

  gtk_widget_show_all (window);

  return window;
}
