/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
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
#include <locale.h>

#include <gtk/gtk.h>

#include "pixmaps.h"
#include "init.h"
#include "sql.h"
#include "smallbox.h"

#define _(_x) gettext(_x)

#define MY_PIXMAPS_DIR PREFIX "/share/gpe-timesheet/pixmaps"
#define PIXMAPS_DIR PREFIX "/share/gpe/pixmaps"

static const guint window_x = 240, window_y = 320;

struct pix my_pix[] = {
  { "new", "new" },
  { "delete", "delete" },
  { "clock", "watch" },
  { "stop_clock", "pause_watch" },
  { "ok", "ok" },
  { NULL, NULL }
};

static void
start_timing(GtkWidget *w, gpointer user_data)
{
  GtkCTree *ct = GTK_CTREE (user_data);
  if (GTK_CLIST (ct)->selection)
    {
      GtkCTreeNode *node = GTK_CTREE_NODE (GTK_CLIST (ct)->selection->data);
      struct pix *px = find_pixmap ("tick");
      gtk_ctree_node_set_pixmap (ct, node, 1, px->pixmap, px->mask);
    }
}

static void
stop_timing(GtkWidget *w, gpointer user_data)
{
  GtkCTree *ct = GTK_CTREE (user_data);
  if (GTK_CLIST (ct)->selection)
    {
      GtkCTreeNode *node = GTK_CTREE_NODE (GTK_CLIST (ct)->selection->data);
      gtk_ctree_node_set_text (ct, node, 1, "");
    }
}

static void
ui_delete_task(GtkWidget *w, gpointer user_data)
{
  GtkCTree *ct = GTK_CTREE (user_data);
  if (GTK_CLIST (ct)->selection)
    {
      GtkCTreeNode *node = GTK_CTREE_NODE (GTK_CLIST (ct)->selection->data);
      struct task *t = gtk_ctree_node_get_row_data (ct, node);
      delete_task (t);
      gtk_ctree_remove_node (ct, node);
    }
}

static void
ui_new_task(GtkWidget *w, gpointer p)
{
  GtkCTree *ctree = GTK_CTREE (p);
  GtkCTreeNode *parent = NULL;
  GtkCTreeNode *node;
  gchar *text[2];
  struct task *pt = NULL;

  if (GTK_CLIST (ctree)->selection)
    {
      gchar *text;
      gboolean leaf;
      parent = GTK_CTREE_NODE (GTK_CLIST (ctree)->selection->data);
      pt = gtk_ctree_node_get_row_data (ctree, parent);
      gtk_ctree_get_node_info (ctree, parent, &text, NULL,
			       NULL, NULL, NULL, NULL, &leaf, NULL);
      if (leaf)
	gtk_ctree_set_node_info (ctree, parent, text, 0,
				 NULL, NULL, NULL, NULL, FALSE, TRUE);
    }
  
  text[0] = smallbox (_("New task"), _("Name"), "");
  text[1] = "";
  if (text[0])
    {
      node = gtk_ctree_insert_node (ctree, parent, NULL,
				    text, 0, NULL, NULL, NULL, NULL,
				    TRUE, TRUE);
      new_task (text[0], pt);
    }
}

static void
load_tasks (GSList *list, GtkCTree *ctree, GtkCTreeNode *parent)
{
  GSList *iter;

  for (iter = list; iter; iter = iter->next)
    {
      struct task *t = iter->data;
      gchar *text[2];
      GtkCTreeNode *node;
      text[0] = t->description;
      text[1] = NULL;
      node = gtk_ctree_insert_node (ctree, parent, NULL,
				    text, 0, NULL, NULL, NULL, NULL,
				    t->children ? FALSE : TRUE, TRUE);
      gtk_ctree_node_set_row_data (ctree, node, t);
      if (t->children)
	load_tasks (t->children, ctree, node);
    }
}

int
main(int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *vbox_top;
  GtkWidget *toolbar;
  GtkWidget *pw;
  GtkWidget *tree;
  GtkWidget *chatter;
  struct pix *p;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);  

  if (load_pixmaps (my_pix) == FALSE)
    exit (1);

  if (sql_start () == FALSE)
    exit (1);

  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);

  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);

  tree = gtk_ctree_new (2, 0);
  chatter = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (chatter), 0.0, 0.5);

  vbox_top = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox_top), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox_top), tree, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox_top), chatter, FALSE, FALSE, 0);

  p = find_pixmap ("new");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("New task"), 
			   _("New task"), _("New task"),
			   pw, ui_new_task, tree);

  p = find_pixmap ("delete");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Delete task"),
			   _("Delete task"), _("Delete task"),
			   pw, ui_delete_task, tree);

  p = find_pixmap ("clock");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Start timing"),
			   _("Start timing"), _("Start timing"),
			   pw, start_timing, tree);
  
  p = find_pixmap ("stop_clock");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Stop timing"),
			   _("Stop timing"), _("Stop timing"),
			   pw, stop_timing, tree);


  gtk_widget_show (toolbar);
  gtk_widget_show (vbox_top);
  gtk_widget_show (tree);
  gtk_widget_show (chatter);

  load_tasks (root, GTK_CTREE (tree), NULL);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_container_add (GTK_CONTAINER (window), vbox_top);
  gtk_widget_set_usize (window, window_x, window_y);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      gtk_main_quit, NULL);

  gtk_widget_realize (window);

  gtk_clist_set_column_width (GTK_CLIST (tree), 0, 
			      tree->allocation.width - 32);

  gtk_widget_show (window);

  gtk_main ();

  return 0;
}
