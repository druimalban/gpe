/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <stdio.h>

#include "interface.h"
#include "support.h"

typedef enum
{
  PAGE,
  GROUP,
  ITEM_MULTI_LINE,
  ITEM_SINGLE_LINE
} edit_thing_type;

struct edit_thing
{
  edit_thing_type type;
  gchar *name;
  GSList *children;
  struct edit_thing *parent;
};

GSList *edit_pages;

typedef struct edit_thing *edit_thing_t;

static void
run_list (GtkCTree *ct, GSList *list, GtkCTreeNode *parent)
{
  GSList *iter;

  for (iter = list; iter; iter = iter->next)
    {
      GtkCTreeNode *node;
      edit_thing_t e = iter->data;
      gchar *text[2];

      text[0] = e->name;
      text[1] = NULL;
 
      node = gtk_ctree_insert_node (ct,
				    parent,
				    NULL,
				    text,
				    4,
				    NULL,
				    NULL,
				    NULL,
				    NULL,
				    (e->type == PAGE || e->type == GROUP) ? FALSE : TRUE,
				    TRUE);

      GTK_CTREE_ROW (node)->row.data = e;

      if (e->children)
	run_list (ct, e->children, node);
    }
}

void
edit_structure (void)
{
  GtkWidget *w = create_structure ();
  GtkWidget *tree = lookup_widget (w, "ctree1");

  run_list (GTK_CTREE (tree), edit_pages, NULL);

  gtk_widget_show (w);
}

edit_thing_t
new_thing (edit_thing_type t, gchar *name, edit_thing_t parent)
{
  edit_thing_t e = g_malloc (sizeof (*e));
  memset (e, 0, sizeof (*e));
  e->type = t;
  e->name = name;
  e->children = NULL;
  e->parent = parent;
  if (parent)
    parent->children = g_slist_append (parent->children, e);
  return e;
}


void
structure_add_clicked (GtkButton       *button,
		       gpointer         user_data)
{
  GtkWidget *w = create_structure_item ();
  GtkCTree *ct = GTK_CTREE (user_data);

  if (GTK_CLIST (ct)->selection)
    {
      GtkCTreeNode *node = GTK_CTREE_NODE (GTK_CLIST (ct)->selection->data);
      edit_thing_t e = GTK_CTREE_ROW (node)->row.data;
    }

  gtk_widget_show (w);
}


void
structure_edit_clicked (GtkButton       *button,
			gpointer         user_data)
{
  GtkCTree *ct = GTK_CTREE (user_data);
  if (GTK_CLIST (ct)->selection)
    {
      GtkCTreeNode *node = GTK_CTREE_NODE (GTK_CLIST (ct)->selection->data);
      GtkWidget *w = create_structure_item ();
      GtkWidget *ww, *radio1, *radio2;
      edit_thing_t e = GTK_CTREE_ROW (node)->row.data;

      radio1 = lookup_widget (w, "radiobutton2");
      radio2 = lookup_widget (w, "radiobutton3");
      
      ww = lookup_widget (w, "entry16");
      gtk_entry_set_text (GTK_ENTRY (ww), e->name);

      ww = lookup_widget (w, "optionmenu1");
      switch (e->type)
	{
	case PAGE:
	  gtk_option_menu_set_history (GTK_OPTION_MENU (ww), 0);
	  gtk_widget_set_sensitive (radio1, FALSE);
	  gtk_widget_set_sensitive (radio2, FALSE);
	  break;

	case GROUP:
	  gtk_widget_set_sensitive (radio1, FALSE);
	  gtk_widget_set_sensitive (radio2, FALSE);
	  gtk_option_menu_set_history (GTK_OPTION_MENU (ww), 1);
	  break;

	case ITEM_SINGLE_LINE:
	  gtk_option_menu_set_history (GTK_OPTION_MENU (ww), 2);
	  gtk_widget_set_sensitive (radio1, TRUE);
	  gtk_widget_set_sensitive (radio2, TRUE);
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio1), TRUE);
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio2), FALSE);
	  break;

	case ITEM_MULTI_LINE:
	  gtk_option_menu_set_history (GTK_OPTION_MENU (ww), 2);
	  gtk_widget_set_sensitive (radio1, TRUE);
	  gtk_widget_set_sensitive (radio2, TRUE);
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio1), FALSE);
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio2), TRUE);
	  break;
	}

      gtk_widget_show (w);
    }
}


void
structure_delete_clicked (GtkButton       *button,
			  gpointer         user_data)
{
  GtkCTree *ct = GTK_CTREE (user_data);
  if (GTK_CLIST (ct)->selection)
    {
      GtkCTreeNode *node = GTK_CTREE_NODE (GTK_CLIST (ct)->selection->data);
      edit_thing_t e = GTK_CTREE_ROW (node)->row.data;
      
      gtk_ctree_remove_node (ct, node);
      
      if (e->parent)
	e->parent->children = g_slist_remove (e->parent->children, e);
      else
	edit_pages = g_slist_remove (edit_pages, e);
    }
}

void
initial_structure (void)
{
  edit_thing_t contact_page = new_thing (PAGE, "Contacts", NULL);
  edit_thing_t address_page = new_thing (PAGE, "Addresses", NULL);

  edit_thing_t phone_group = new_thing (GROUP, "Phone", contact_page);
  edit_thing_t internet_group = new_thing (GROUP, "Internet", contact_page);

  new_thing (ITEM_SINGLE_LINE, "Home", phone_group);
  new_thing (ITEM_SINGLE_LINE, "Work", phone_group);

  new_thing (ITEM_SINGLE_LINE, "Mail", internet_group);
  new_thing (ITEM_SINGLE_LINE, "Web", internet_group);

  new_thing (ITEM_MULTI_LINE, "Home", address_page);
  new_thing (ITEM_MULTI_LINE, "Work", address_page);

  edit_pages = g_slist_append (edit_pages, contact_page);
  edit_pages = g_slist_append (edit_pages, address_page);
}

static void
build_children (GtkWidget *vbox, GSList *children)
{
  GSList *child;
  for (child = children; child; child = child->next)
    {
      edit_thing_t e = child->data;
      GtkWidget *w, *ww;
      
      switch (e->type)
	{
	case GROUP:
	  w = gtk_frame_new (e->name);
	  ww = gtk_vbox_new (FALSE, 0);
	  gtk_container_add (GTK_CONTAINER (w), ww);
	  gtk_container_set_border_width (GTK_CONTAINER (w), 4);
	  build_children (ww, e->children);
	  break;

	case ITEM_MULTI_LINE:
	  w = gtk_frame_new (e->name);
	  ww = gtk_text_new (NULL, NULL);
	  gtk_container_add (GTK_CONTAINER (w), ww);
	  gtk_container_set_border_width (GTK_CONTAINER (w), 4);
	  break;

	case ITEM_SINGLE_LINE:
	  w = gtk_hbox_new (FALSE, 4);
	  gtk_box_pack_start (GTK_BOX (w), gtk_label_new (e->name), 
			      FALSE, FALSE, 0);
	  gtk_box_pack_start (GTK_BOX (w), gtk_entry_new (), 
			      TRUE, TRUE, 0);
	  break;

	default:
	  abort ();
	}
      
      gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 4);
    }
}

GtkWidget *
build_edit_page (void)
{
  GtkWidget *book = gtk_notebook_new ();
  GSList *page;
  
  for (page = edit_pages; page; page = page->next)
    {
      edit_thing_t e = page->data;
      GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
      GtkWidget *label = gtk_label_new (e->name);

      build_children (vbox, e->children);

      gtk_notebook_append_page (GTK_NOTEBOOK (book), vbox, label);
    }

  return book;
}

void
try_it (void)
{
  GtkWidget *w = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  
  gtk_container_add (GTK_CONTAINER (w), build_edit_page ());
  
  gtk_widget_show_all (w);
}
