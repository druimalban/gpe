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
#include "pixmaps.h"
#include "gtkdatecombo.h"
#include "callbacks.h"
#include "smallbox.h"

GSList *edit_pages;

static void
structure_new_page (GtkWidget *widget,
		    gpointer d)
{
  gchar *t = smallbox (_("New page"), _("Title"), "");
}

static void
structure_new_group (GtkWidget *widget,
		    gpointer d)
{
  gchar *t = smallbox (_("New group"), _("Title"), "");
}

static void
structure_new_field (GtkWidget *widget,
		    gpointer d)
{
}

void
structure_edit_clicked (GtkButton       *button,
			gpointer         user_data)
{
  GtkCTree *ct = GTK_CTREE (user_data);
#if 0
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
#endif
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

static void
build_edit_tree (GtkCTree *ct, GSList *list, GtkCTreeNode *parent)
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
	build_edit_tree (ct, e->children, node);
    }
}

GtkWidget *
edit_structure (void)
{
  GtkWidget *tree = gtk_ctree_new (2, 0);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget *toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, 
					GTK_TOOLBAR_ICONS);
  struct pix *p;
  GtkWidget *pw;

  gtk_widget_show (vbox);
  gtk_widget_show (scrolled);
  gtk_widget_show (tree);
  gtk_widget_show (toolbar);

  p = find_pixmap ("notebook");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), "New page", 
			   "New page", "New page", 
			   pw, structure_new_page, tree);

  p = find_pixmap ("frame");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), "New group", 
			   "New group", "New group", 
			   pw, structure_new_group, tree);

  p = find_pixmap ("entry");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), "New field", 
			   "New field", "New field", 
			   pw, structure_new_field, tree);

  p = find_pixmap ("delete");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Delete"), 
			   _("Delete"), _("Delete"), pw, NULL, NULL);

#if 0
  p = find_pixmap ("save");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), "Save", 
			   "Save", "Save", pw, NULL, NULL);
#endif

  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (scrolled), tree);
  gtk_clist_set_column_width (GTK_CLIST (tree), 0, 80);
  gtk_clist_set_column_width (GTK_CLIST (tree), 1, 80);
  gtk_clist_column_titles_hide (GTK_CLIST (tree));

  build_edit_tree (GTK_CTREE (tree), edit_pages, NULL);

  return vbox;
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
      if (e->name)
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
      if (e->name)
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
      if (e->name)
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
      if (e->name)
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
