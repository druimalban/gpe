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

#include <parser.h>

#include "errorbox.h"

#include "interface.h"
#include "support.h"
#include "structure.h"

static GSList *edit_pages;

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
  new_thing (ITEM_SINGLE_LINE, "Mobile", phone_group);
  new_thing (ITEM_SINGLE_LINE, "Fax", phone_group);

  new_thing (ITEM_SINGLE_LINE, "Mail", internet_group);
  new_thing (ITEM_SINGLE_LINE, "Web", internet_group);

  new_thing (ITEM_MULTI_LINE, "Home", address_page);
  new_thing (ITEM_MULTI_LINE, "Work", address_page);

  edit_pages = g_slist_append (edit_pages, contact_page);
  edit_pages = g_slist_append (edit_pages, address_page);
}

static void
add_tag (guint tag, GtkWidget *w, GtkWidget *pw)
{
  GSList *tags;
  gtk_object_set_data (GTK_OBJECT (w), "db-tag", (gpointer)tag);

  tags = gtk_object_get_data (GTK_OBJECT (pw), "tag-widgets");
  tags = g_slist_append (tags, w);
  gtk_object_set_data (GTK_OBJECT (pw), "tag-widgets", tags);
}

static void
pop_singles (GtkWidget *vbox, GSList *list, GtkWidget *pw)
{
  if (list)
    {
      guint l = g_slist_length (list);
      GtkWidget *table = gtk_table_new (l, 2, FALSE);
      guint x = 0;
      
      while (list)
	{
	  GSList *next = list->next;
	  edit_thing_t e = list->data;
	  GtkWidget *w = gtk_entry_new ();

	  add_tag (e->tag, w, pw);

	  gtk_table_attach (GTK_TABLE (table),
			    gtk_label_new (e->name),
			    0, 1, x, x + 1,
			    0, 0, 0, 0);
	  gtk_table_attach (GTK_TABLE (table),
			    w,
			    1, 2, x, x + 1,
			    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			    0, 0, 0);

	  g_slist_free_1 (list);
	  list = next;
	  x++;
	}

      gtk_table_set_col_spacings (GTK_TABLE (table), 2);
      gtk_container_set_border_width (GTK_CONTAINER (table), 2);

      gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 4);
    }
}

static void
build_children (GtkWidget *vbox, GSList *children, GtkWidget *pw)
{
  GSList *child;
  GSList *singles = NULL;

  for (child = children; child; child = child->next)
    {
      edit_thing_t e = child->data;
      GtkWidget *w, *ww;
      
      switch (e->type)
	{
	case GROUP:
	  pop_singles (vbox, singles, pw);
	  singles = NULL;
	  w = gtk_frame_new (e->name);
	  ww = gtk_vbox_new (FALSE, 0);
	  gtk_container_add (GTK_CONTAINER (w), ww);
	  gtk_container_set_border_width (GTK_CONTAINER (w), 2);
	  build_children (ww, e->children, pw);
	  gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 4);
	  break;

	case ITEM_MULTI_LINE:
	  pop_singles (vbox, singles, pw);
	  singles = NULL;
	  w = gtk_frame_new (e->name);
	  ww = gtk_text_new (NULL, NULL);
	  gtk_text_set_editable (GTK_TEXT (ww), TRUE);
	  gtk_container_add (GTK_CONTAINER (w), ww);
	  gtk_container_set_border_width (GTK_CONTAINER (w), 2);
	  gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 4);
	  add_tag (e->tag, pw, ww);
	  break;

	case ITEM_SINGLE_LINE:
	  singles = g_slist_append (singles, e);
	  break;

	default:
	  abort ();
	}
    }

  pop_singles (vbox, singles, pw);
  singles = NULL;
}

GtkWidget *
edit_window (void)
{
  GtkWidget *w = create_edit ();
  GtkWidget *book = lookup_widget (w, "notebook2");
  GSList *page;
  GtkWidget *text, *notes_label;

  for (page = edit_pages; page; page = page->next)
    {
      edit_thing_t e = page->data;
      GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
      GtkWidget *label = gtk_label_new (e->name);

      build_children (vbox, e->children, w);

      gtk_widget_show_all (vbox);
      gtk_widget_show (label);

      gtk_notebook_append_page (GTK_NOTEBOOK (book), vbox, label);
    }

  text = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (text), TRUE);
  gtk_widget_show (text);
  notes_label = gtk_label_new ("Notes");
  gtk_widget_show (notes_label);

  gtk_notebook_append_page (GTK_NOTEBOOK (book), text, notes_label);  

  return w;
}

static void
print_structure_1 (FILE *fp, edit_thing_t e, int level)
{
  GSList *iter;
  guint i;

  for (i = 0; i < level; i++)
    fputs ("  ", fp);

  switch (e->type)
    {
    case PAGE:
      fprintf (fp, "<page>\n");
      for (i = 0; i <= level; i++)
	fputs ("  ", fp);
      fprintf (fp, "<label>%s</label>\n", e->name);
      for (iter = e->children; iter; iter = iter->next)
	print_structure_1 (fp, iter->data, level + 1);
      for (i = 0; i < level; i++)
	fputs ("  ", fp);
      fprintf (fp, "</page>\n");
      break;

    case GROUP:
      fprintf (fp, "<group>\n");
      for (i = 0; i <= level; i++)
	fputs ("  ", fp);
      fprintf (fp, "<label>%s</label>\n", e->name);
      for (iter = e->children; iter; iter = iter->next)
	print_structure_1 (fp, iter->data, level + 1);
      for (i = 0; i < level; i++)
	fputs ("  ", fp);
      fprintf (fp, "</group>\n");
      break;

    case ITEM_SINGLE_LINE:
      fprintf (fp, "<single-item>\n");
      for (i = 0; i <= level; i++)
	fputs ("  ", fp);
      fprintf (fp, "<tag>%d</tag>\n", e->tag);
      for (i = 0; i <= level; i++)
	fputs ("  ", fp);
      fprintf (fp, "<label>%s</label>\n", e->name);
      for (i = 0; i < level; i++)
	fputs ("  ", fp);
      fprintf (fp, "</single-item>\n");
      break;

    case ITEM_MULTI_LINE:
      fprintf (fp, "<multi-item>\n");
      for (i = 0; i <= level; i++)
	fputs ("  ", fp);
      fprintf (fp, "<tag>%d</tag>\n", e->tag);
      for (i = 0; i <= level; i++)
	fputs ("  ", fp);
      fprintf (fp, "<label>%s</label>\n", e->name);
      for (i = 0; i < level; i++)
	fputs ("  ", fp);
      fprintf (fp, "</multi-item>\n");
      break;
    }
}

void
print_structure (FILE *fp)
{
  GSList *page;

  fputs ("<layout>\n", fp);

  for (page = edit_pages; page; page = page->next)
    {
      edit_thing_t e = page->data;

      print_structure_1 (fp, e, 1);
    }

  fputs ("</layout>\n", fp);
}

static void
structure_parse_xml_item (xmlDocPtr doc, xmlNodePtr cur, 
			  edit_thing_type t, edit_thing_t parent)
{
  edit_thing_t e = new_thing (t, NULL, parent);

  while (cur)
    {
      if (!xmlStrcmp (cur->name, "label"))
	e->name = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);

      if (!xmlStrcmp (cur->name, "tag"))
	{
	  gchar *s = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
	  e->tag = atoi (s);
	}

      cur = cur->next;
    }
}

static void
structure_parse_xml_group (xmlDocPtr doc, xmlNodePtr cur, 
			   edit_thing_t parent)
{
  edit_thing_t e = new_thing (GROUP, NULL, parent);

  while (cur)
    {
      if (!xmlStrcmp (cur->name, "label"))
	e->name = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);

      if (!xmlStrcmp (cur->name, "multi-item"))
	structure_parse_xml_item (doc, cur->xmlChildrenNode, 
				  ITEM_MULTI_LINE, e);
	
      if (!xmlStrcmp (cur->name, "single-item"))
	structure_parse_xml_item (doc, cur->xmlChildrenNode, 
				  ITEM_SINGLE_LINE, e);

      cur = cur->next;
    }
}

static void
structure_parse_xml_page (xmlDocPtr doc, xmlNodePtr cur)
{
  edit_thing_t e = new_thing (PAGE, NULL, NULL);

  while (cur)
    {
      if (!xmlStrcmp (cur->name, "label"))
	e->name = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);

      if (!xmlStrcmp (cur->name, "group"))
	structure_parse_xml_group (doc, cur->xmlChildrenNode, e);
	
      if (!xmlStrcmp (cur->name, "multi-item"))
	structure_parse_xml_item (doc, cur->xmlChildrenNode, 
				  ITEM_MULTI_LINE, e);
	
      if (!xmlStrcmp (cur->name, "single-item"))
	structure_parse_xml_item (doc, cur->xmlChildrenNode, 
				  ITEM_SINGLE_LINE, e);

      cur = cur->next;
    }

  edit_pages = g_slist_append (edit_pages, e);
}

static void
structure_parse_xml (xmlDocPtr doc, xmlNodePtr cur)
{
  while (cur)
    {
      if (!xmlStrcmp (cur->name, "page"))
	structure_parse_xml_page (doc, cur->xmlChildrenNode);
	
      cur = cur->next;
    }
}

gboolean
read_structure (gchar *name)
{
  xmlDocPtr doc = xmlParseFile (name);
  xmlNodePtr cur;

  if (doc == NULL)
    {
      gpe_perror_box (name);
      return FALSE;
    }

  xmlCleanupParser ();

  cur = xmlDocGetRootElement (doc);
  if (cur == NULL)
    {
      gpe_error_box ("Layout description is empty");
      xmlFreeDoc (doc);
      return FALSE;
    }

  if (xmlStrcmp (cur->name, "layout")) 
    {
      gpe_error_box ("Layout description has wrong document type");
      xmlFreeDoc (doc);
      return FALSE;
    }

  edit_pages = NULL;

  structure_parse_xml (doc, cur->xmlChildrenNode);
      
  xmlFreeDoc (doc);

  return TRUE;
}
