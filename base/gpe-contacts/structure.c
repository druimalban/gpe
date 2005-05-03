/*
 * Copyright (C) 2002, 2003, 2005 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <gpe/pixmaps.h>
#include <gpe/gtkdatecombo.h>
#include <gpe/smallbox.h>
#include <gpe/errorbox.h>
#include <gpe/question.h>

#include "support.h"
#include "structure.h"

GSList *edit_pages;
GList *well_known_tags;

#define LAYOUT_NAME "/.gpe/contacts-layout.xml"

edit_thing_t new_thing (edit_thing_type t, gchar *name, edit_thing_t parent);

static void
insert_new_thing (GtkCTree *ct,
		  gchar *name,
		  edit_thing_type type,
		  gchar *tag)
{
  GtkCTreeNode *sibling = NULL, *parent = NULL, *node;
  edit_thing_t e_sibling = NULL, e_parent = NULL;
  edit_thing_t n;
  GSList **list;
  gchar *line_info[2];

  if (GTK_CLIST (ct)->selection)
    {
      GtkCTreeNode *node = GTK_CTREE_NODE (GTK_CLIST (ct)->selection->data);
      edit_thing_t e = GTK_CTREE_ROW (node)->row.data;
      
      sibling = node;
      e_sibling = e;
      
      while (type < ITEM_MULTI_LINE && e_sibling->type > type)
	{
	  sibling = GTK_CTREE_ROW (sibling)->parent;
	  e_sibling = e_sibling->parent;
	}

      if (e_sibling->type < type)
	{
	  e_parent = e_sibling;
	  parent = sibling;
	  e_sibling = NULL;
	  sibling = NULL;
	}
      else
	{
	  e_parent = e_sibling->parent;
	  parent = GTK_CTREE_ROW (sibling)->parent;
	}
    }

  if (e_parent)
    list = &e_parent->children;
  else
    list = &edit_pages;

  n = new_thing (type, name, NULL);
  n->parent = e_parent;
  n->tag = tag;

  line_info[0] = name;
  line_info[1] = NULL;
  
  node = gtk_ctree_insert_node (ct, parent, sibling, line_info,
				4, NULL, NULL, NULL, NULL,
				FALSE, TRUE);

  gtk_ctree_node_set_row_data (ct, node, n);

  if (e_sibling == NULL)
    {
      *list = g_slist_append (*list, n);
    }
  else
    {
      gint index = g_slist_index (*list, e_sibling);
      *list = g_slist_insert (*list, n, index);
    }
}

static void
structure_new_page (GtkWidget *widget,
		    gpointer user_data)
{
  gchar *t = smallbox (_("New page"), _("Title"), "");
  GtkCTree *ct = GTK_CTREE (user_data);
  
  if (t)
    {
      if (t[0])
	insert_new_thing (ct, t, PAGE, NULL);
      else
	g_free (t);
    }
}

static void
structure_new_group (GtkWidget *widget,
		    gpointer user_data)
{
  GtkCTree *ct = GTK_CTREE (user_data);
  if (GTK_CLIST (ct)->selection)
    {
      gchar *t = smallbox (_("New group"), _("Title"), "");

      if (t)
	{
	  if (t[0])
	    insert_new_thing (ct, t, GROUP, NULL);
	  else
	    g_free (t);
	}
    }
  else
    {
      gpe_error_box (_("No container selected"));
    }
}

static void
structure_new_field (GtkWidget *widget,
		    gpointer user_data)
{
  GtkCTree *ct = GTK_CTREE (user_data);
  if (GTK_CLIST (ct)->selection)
    {
      struct box_desc2 bd[3];
      memset (&bd, 0, sizeof (bd));
      bd[0].label = _("Title");
      bd[1].label = _("Tag");
      bd[1].suggestions = well_known_tags;
      smallbox_x2 (_("New field"), bd);

      if (bd[0].value && bd[1].value && bd[0].value[0] && bd[1].value[0])
         insert_new_thing (ct, bd[0].value, ITEM_SINGLE_LINE, bd[1].value);
      else
        {
          if (bd[0].value)
            g_free (bd[0].value);
          if (bd[1].value)
            g_free (bd[1].value);
        }
    }
  else
    {
      gpe_error_box (_("No container selected"));
    }
}

static void
structure_delete_item (GtkButton       *button,
		       gpointer         user_data)
{
  GtkCTree *ct = GTK_CTREE (user_data);
  if (GTK_CLIST (ct)->selection
      && (gpe_question_ask (_("Really delete this field?"), _("Confirm"), 
			    "question", _("Delete"), "delete", _("Cancel"), "cancel", NULL) == 0))

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
 
      node = gtk_ctree_insert_node (ct, parent, NULL, text,
				    4, NULL, NULL, NULL, NULL,
				    (e->type == PAGE 
				     || e->type == GROUP) ? FALSE : TRUE,
				    TRUE);
      
      GTK_CTREE_ROW (node)->row.data = e;

      if (e->children)
	build_edit_tree (ct, e->children, node);
    }
}

GtkWidget *
edit_structure (void)
{
  GtkWidget *tree = gtk_ctree_new (1, 0);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget *toolbar;
  GtkWidget *pw;

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

  pw = gtk_image_new_from_pixbuf (gpe_find_icon_scaled ("notebook",
							gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar))));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Page"), 
			   _("New page"), _("Tap here to add a page."), pw, 
			   GTK_SIGNAL_FUNC (structure_new_page), tree);

  pw = gtk_image_new_from_pixbuf (gpe_find_icon_scaled ("frame",
							gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar))));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), "New group", 
			   "New group", "New group", pw, 
			   GTK_SIGNAL_FUNC (structure_new_group), tree);

  pw = gtk_image_new_from_pixbuf (gpe_find_icon_scaled ("entry",
							gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar))));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), "New field", 
			   "New field", "New field", pw, 
			   GTK_SIGNAL_FUNC (structure_new_field), tree);
  
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_DELETE,
			    _("Delete item"), _("Tap here to delete the selected item."),
			    G_CALLBACK (structure_delete_item), tree, -1);

  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (scrolled), tree);
  gtk_clist_column_titles_hide (GTK_CLIST (tree));

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

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
  e->hidden = FALSE;
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
      fprintf (fp, "<tag>%s</tag>\n", e->tag);
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
      fprintf (fp, "<tag>%s</tag>\n", e->tag);
      for (i = 0; i <= level; i++)
	fputs ("  ", fp);
      if (e->name)
	fprintf (fp, "<label>%s</label>\n", e->name);
      for (i = 0; i < level; i++)
	fputs ("  ", fp);
      fprintf (fp, "</multi-item>\n");
      break;
    
    case ITEM_DATE:
      fprintf (fp, "<date-item>\n");
      for (i = 0; i <= level; i++)
	fputs ("  ", fp);
      fprintf (fp, "<tag>%s</tag>\n", e->tag);
      for (i = 0; i <= level; i++)
	fputs ("  ", fp);
      if (e->name)
	fprintf (fp, "<label>%s</label>\n", e->name);
      for (i = 0; i < level; i++)
	fputs ("  ", fp);
      fprintf (fp, "</date-item>\n");
      break;
      
    case ITEM_IMAGE:
      fprintf (fp, "<image-item>\n");
      for (i = 0; i <= level; i++)
	fputs ("  ", fp);
      fprintf (fp, "<tag>%s</tag>\n", e->tag);
      for (i = 0; i <= level; i++)
	fputs ("  ", fp);
      if (e->name)
	fprintf (fp, "<label>%s</label>\n", e->name);
      for (i = 0; i < level; i++)
	fputs ("  ", fp);
      fprintf (fp, "</image-item>\n");
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

struct xml_parser_context
{
  GQueue *queue;
  GQueue *elt_queue;
};

edit_thing_t
new_element (struct xml_parser_context *c, edit_thing_type t, gchar *name, edit_thing_t parent)
{
  edit_thing_t e;

  e = new_thing (t, name, parent);

  g_queue_push_head (c->queue, e);

  return e;
}

static void 
xml_parser_start_element (GMarkupParseContext *context,
			  const gchar         *element_name,
			  const gchar        **attribute_names,
			  const gchar        **attribute_values,
			  gpointer             user_data,
			  GError             **error)
{
  struct xml_parser_context *c = (struct xml_parser_context *)user_data;
  const gchar *cur_elem;

  cur_elem = g_queue_peek_head (c->elt_queue);
  
  if (cur_elem == NULL)
    {
      if (!strcmp (element_name, "layout"))
	;
      else
	g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
		     "Unknown element \"%s\" at top level", element_name);
    }
  else if (!strcmp (cur_elem, "layout"))
    {
      if (!strcmp (element_name, "page"))
	{
	  edit_thing_t e;
	  e = new_element (c, PAGE, NULL, NULL);
	  edit_pages = g_slist_append (edit_pages, e);
	}
      else
	goto bad;
    }
  else if (!strcmp (cur_elem, "page"))
    {
      edit_thing_t e;

      e = g_queue_peek_head (c->queue);

      if (!strcmp (element_name, "label"))
	;

      else if (!strcmp (element_name, "group"))
	new_element (c, GROUP, NULL, e);

      else if (!strcmp (element_name, "multi-item"))
	new_element (c, ITEM_MULTI_LINE, NULL, e);

      else if (!strcmp (element_name, "single-item"))
	new_element (c, ITEM_SINGLE_LINE, NULL, e);

      else if (!strcmp (element_name, "date-item"))
	new_element (c, ITEM_DATE, NULL, e);
      
      else if (!strcmp (element_name, "image-item"))
	new_element (c, ITEM_IMAGE, NULL, e);
    }
  else if (!strcmp (cur_elem, "group"))
    {
      edit_thing_t e;

      e = g_queue_peek_head (c->queue);

      if (!strcmp (element_name, "label"))
	;

      else if (!strcmp (element_name, "multi-item"))
	new_element (c, ITEM_MULTI_LINE, NULL, e);

      else if (!strcmp (element_name, "single-item"))
	new_element (c, ITEM_SINGLE_LINE, NULL, e);

      else if (!strcmp (element_name, "date-item"))
	new_element (c, ITEM_DATE, NULL, e);
      
      else if (!strcmp (element_name, "image-item"))
	new_element (c, ITEM_IMAGE, NULL, e);
    }
  else if (!strcmp (cur_elem, "single-item")
	   || !strcmp (cur_elem, "multi-item")
	   || !strcmp (cur_elem, "date-item")
	   || !strcmp (cur_elem, "image-item"))
    {
      if (!strcmp (element_name, "tag")
	  || !strcmp (element_name, "label"))
	;
      else
	goto bad;
    }
  else
    {
      goto bad;
    }

  g_queue_push_head (c->elt_queue, element_name);

  return;

 bad:
  g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT, 
	       "Unknown element \"%s\" inside <%s>", element_name, cur_elem);      
}

static void 
xml_parser_end_element (GMarkupParseContext *context,
			const gchar         *element_name,
			gpointer             user_data,
			GError             **error)
{
  struct xml_parser_context *c = (struct xml_parser_context *)user_data;
  const gchar *cur_elem;

  cur_elem = g_queue_pop_head (c->elt_queue);

  if (!strcmp (cur_elem, "layout") || !strcmp (cur_elem, "label") || !strcmp (cur_elem, "tag"))
    return;

  g_queue_pop_head (c->queue);
}

static void
xml_parser_text (GMarkupParseContext *context,
		 const gchar         *text,
		 gsize                text_len,  
		 gpointer             user_data,
		 GError             **error)
{
  struct xml_parser_context *c = (struct xml_parser_context *)user_data;
  const gchar *cur_elem;
  edit_thing_t e;
  
  cur_elem = g_markup_parse_context_get_element (context);

  e = g_queue_peek_head (c->queue);

  if (!strcmp (cur_elem, "label"))
    {
      e->name = g_strndup (text, text_len);
      if (!strcmp (e->name, "hidden"))
	e->hidden = TRUE;
    }
  else if (!strcmp (cur_elem, "tag"))
    {
      e->tag = g_strndup (text, text_len);
    }
}

static GMarkupParser 
xml_parser = 
  {
    start_element: xml_parser_start_element,
    end_element: xml_parser_end_element,
    text: xml_parser_text
  };

static void
free_ctx (gpointer p)
{
  struct xml_parser_context *c = p;

  g_queue_free (c->queue);
  g_queue_free (c->elt_queue);

  g_free (p);
}

gboolean
read_structure (gchar *name)
{
  GMarkupParseContext *ctx;
  struct xml_parser_context *user_ctx;
  FILE *fp;

  fp = fopen (name, "r");
  if (!fp)
    {
      gpe_perror_box (name);
      return FALSE;
    }

  user_ctx = g_malloc (sizeof (*user_ctx));

  user_ctx->queue = g_queue_new ();
  user_ctx->elt_queue = g_queue_new ();

  ctx = g_markup_parse_context_new (&xml_parser, 0, user_ctx, free_ctx);

  edit_pages = NULL;

  while (! feof (fp))
    {
      char buf[1024];
      int n;
      GError *err = NULL;

      n = fread (buf, 1, 1024, fp);
      if (g_markup_parse_context_parse (ctx, buf, n, &err) == FALSE)
	{
	  gpe_error_box (err->message);
	  g_error_free (err);

	  g_markup_parse_context_free (ctx);
	  
	  fclose (fp);

	  return FALSE;
	}
    }

  g_markup_parse_context_free (ctx);

  fclose (fp);
  
  return TRUE;
}

void
load_well_known_tags (void)
{
  static const gchar *filename = PREFIX "/share/gpe-contacts/well-known-tags";
  FILE *fp = fopen (filename, "r");
  if (fp)
    {
      char buf[256];
      while (fgets (buf, sizeof (buf), fp))
	{
	  char *s = buf, *h = strchr (buf, '#');
	  if (h)
	    *h = 0;	// dump comments
	  while (isspace (*s))
	    s++;	// dump leading whitespace
	  h = s;
	  while (*h && !isspace (*h))
	    h++;
	  *h = 0;	// dump trailing whitespace
	  if (*s)
	    well_known_tags = g_list_append (well_known_tags, g_ascii_strup (s, -1));
	}
      fclose (fp);
    }
}

gboolean
load_structure (gchar* default_structure)
{
  gchar *buf;
  size_t len;
  char *home = getenv ("HOME");
  gboolean rc = TRUE;
  if (home == NULL)
    home = "";
  
  len = strlen (home) + strlen (LAYOUT_NAME) + 1;
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, LAYOUT_NAME);
  
  if (access (buf, F_OK))
    read_structure (default_structure);
  else
    rc = read_structure (buf);
  
  g_free (buf);
  return rc;
}
