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

#include <gpe/pixmaps.h>
#include <gpe/render.h>
#include <gpe/init.h>
#include <gpe/smallbox.h>
#include <gpe/picturebutton.h>

#include "sql.h"

#define _(_x) gettext(_x)

static const guint window_x = 240, window_y = 320;

struct gpe_icon my_icons[] = {
  { "new", "new" },
  { "delete", },
  { "clock", },
  { "stop_clock", },
  { "tick", },
  { "ok" },
  { "cancel" },
  { "gpe-timesheet", PREFIX "/share/pixmaps/gpe-timesheet.png" },
  { "edit" },
  { NULL, NULL }
};

static GdkWindow *top_level_window;

static void
mark_started (GtkCTree *ct, GtkCTreeNode *node)
{
  GdkPixmap *pixmap;
  GdkBitmap *bitmap;
  gpe_find_icon_pixmap ("tick", &pixmap, &bitmap);
  gtk_ctree_node_set_pixmap (ct, node, 1, pixmap, bitmap);
}

static void
confirm_click_ok (GtkWidget *widget, gpointer p)
{
  gtk_main_quit ();
}

static void
confirm_click_cancel (GtkWidget *widget, gpointer p)
{
  gtk_widget_destroy (GTK_WIDGET (p));
}

static void
confirm_note_destruction (GtkWidget *widget, gpointer p)
{
  gboolean *b = (gboolean *)p;

  if (*b == FALSE)
    {
      *b = TRUE;
      
      gtk_main_quit ();
    }
}

static gboolean
confirm_dialog (gchar **text, gchar *action, gchar *action2)
{
  GtkWidget *w = gtk_dialog_new ();
  gboolean destroyed = FALSE;
  GtkWidget *buttonok, *buttoncancel;
  GdkPixbuf *pixbuf;
  GtkWidget *icon;
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *vbox2 = gtk_vbox_new (FALSE, 0);
  GtkWidget *action_label, *action2_label;
  GtkWidget *frame, *entry;

  gtk_widget_realize (w);
  gdk_window_set_transient_for (w->window, top_level_window);

  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
  gtk_widget_show (vbox2);

  if (action)
    {
      action_label = gtk_label_new (action);
      gtk_box_pack_start (GTK_BOX (vbox), action_label, FALSE, FALSE, 0);
      gtk_widget_show (action_label);
      gtk_misc_set_alignment (GTK_MISC (action_label), 0.0, 0.5);
    }
  action2_label = gtk_label_new (action2);
  gtk_box_pack_start (GTK_BOX (vbox), action2_label, FALSE, FALSE, 0);
  gtk_widget_show (action2_label);

  gtk_misc_set_alignment (GTK_MISC (action2_label), 0.0, 0.5);
  
  gtk_box_pack_start (GTK_BOX (vbox2), vbox, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 8);

  pixbuf = gpe_find_icon ("gpe-timesheet");
  if (pixbuf)
    {
      icon = gpe_render_icon (w->style, pixbuf);
      gtk_box_pack_end (GTK_BOX (hbox), icon, FALSE, FALSE, 0);
      gtk_widget_show (icon);
    }

  frame = gtk_frame_new (_("Notes"));
  gtk_widget_show (frame);
#if GTK_MAJOR_VERSION < 2
  entry = gtk_text_new (NULL, NULL);
#else
  entry = gtk_text_view_new ();
#endif
  gtk_widget_show (entry);
  gtk_container_add (GTK_CONTAINER (frame), entry);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (w)->vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (w)->vbox), frame, TRUE, TRUE, 0);

  buttonok = gpe_picture_button (w->style, _("OK"), "ok");
  buttoncancel = gpe_picture_button (w->style, _("Cancel"), "cancel");

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (w)->action_area), 
		      buttonok, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (w)->action_area), 
		      buttoncancel, TRUE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked", 
		      GTK_SIGNAL_FUNC (confirm_click_ok), NULL);
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked", 
		      GTK_SIGNAL_FUNC (confirm_click_cancel), w);

  gtk_signal_connect (GTK_OBJECT (w), "destroy",
		      (GtkSignalFunc)confirm_note_destruction, &destroyed);

  gtk_window_set_modal (GTK_WINDOW (w), TRUE);
  gtk_widget_show (w);

  gtk_widget_grab_focus (GTK_WIDGET (entry));

  gtk_main ();

  if (destroyed)
    return FALSE;

#if GTK_MAJOR_VERSION < 2
  *text = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
#else
  {
    GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (entry));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds (buf, &start, &end);
    *text = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
  }
#endif

  destroyed = TRUE;
  gtk_widget_destroy (w);

  return TRUE;
}

static void
start_timing (GtkWidget *w, gpointer user_data)
{
  GtkCTree *ct = GTK_CTREE (user_data);
  if (GTK_CLIST (ct)->selection)
    {
      GtkCTreeNode *node = GTK_CTREE_NODE (GTK_CLIST (ct)->selection->data);
      struct task *t;
      gchar *text;
      t = gtk_ctree_node_get_row_data (ct, node);
      if (confirm_dialog (&text, _("Clocking in:"), t->description))
	{
	  log_entry (START, time (NULL), t, text);
	  mark_started (ct, node);
	  g_free (text);
	}
    }
}

static void
stop_timing (GtkWidget *w, gpointer user_data)
{
  GtkCTree *ct = GTK_CTREE (user_data);
  if (GTK_CLIST (ct)->selection)
    {
      GtkCTreeNode *node = GTK_CTREE_NODE (GTK_CLIST (ct)->selection->data);
      struct task *t;
      gchar *text;
      t = gtk_ctree_node_get_row_data (ct, node);
      if (confirm_dialog (&text, _("Clocking out:"), t->description))
	{
	  gtk_ctree_node_set_text (ct, node, 1, "");
	  log_entry (STOP, time (NULL), t, text);
	  g_free (text);
	}
    }
}

static void
note (GtkWidget *w, gpointer user_data)
{
  GtkCTree *ct = GTK_CTREE (user_data);
  if (GTK_CLIST (ct)->selection)
    {
      GtkCTreeNode *node = GTK_CTREE_NODE (GTK_CLIST (ct)->selection->data);
      struct task *t;
      gchar *text;
      t = gtk_ctree_node_get_row_data (ct, node);
      if (confirm_dialog (&text, NULL, t->description))
	{
	  log_entry (NOTE, time (NULL), t, text);
	  g_free (text);
	}
    }
}

static void
ui_delete_task (GtkWidget *w, gpointer user_data)
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
ui_new_task (GtkWidget *w, gpointer p)
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
      struct task *t;
      node = gtk_ctree_insert_node (ctree, parent, NULL,
				    text, 0, NULL, NULL, NULL, NULL,
				    TRUE, TRUE);
      t = new_task (text[0], pt);
      gtk_ctree_node_set_row_data (ctree, node, t);
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
      if (t->started)
	mark_started (ctree, node);
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
  GdkPixbuf *p;
  GdkPixmap *pmap;
  GdkBitmap *bmap;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  if (sql_start () == FALSE)
    exit (1);

#if GTK_MAJOR_VERSION < 2
  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
#else
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
#endif

  tree = gtk_ctree_new (2, 0);
  chatter = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (chatter), 0.0, 0.5);

  vbox_top = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox_top), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox_top), tree, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox_top), chatter, FALSE, FALSE, 0);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (window);
  top_level_window = window->window;

  p = gpe_find_icon ("new");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("New task"), 
			   _("New task"), _("New task"),
			   pw, (GtkSignalFunc)ui_new_task, tree);

  p = gpe_find_icon ("delete");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Delete task"),
			   _("Delete task"), _("Delete task"),
			   pw, (GtkSignalFunc)ui_delete_task, tree);

  p = gpe_find_icon ("clock");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Clock on"),
			   _("Clock on"), _("Clock on"),
			   pw, (GtkSignalFunc)start_timing, tree);
  
  p = gpe_find_icon ("stop_clock");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Clock off"),
			   _("Clock off"), _("Clock off"),
			   pw, (GtkSignalFunc)stop_timing, tree);

  p = gpe_find_icon ("edit");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Note"),
			   _("Note"), _("Note"),
			   pw, (GtkSignalFunc)note, tree);

  gtk_widget_show (toolbar);
  gtk_widget_show (vbox_top);
  gtk_widget_show (tree);
  gtk_widget_show (chatter);

  scan_logs (root);
  load_tasks (root, GTK_CTREE (tree), NULL);
  
  gtk_container_add (GTK_CONTAINER (window), vbox_top);
  gtk_widget_set_usize (window, window_x, window_y);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      gtk_main_quit, NULL);

  gtk_widget_realize (window);
  if (gpe_find_icon_pixmap ("gpe-timesheet", &pmap, &bmap))
    gdk_window_set_icon (window->window, NULL, pmap, bmap);
  gtk_widget_show (window);

  gtk_clist_set_column_width (GTK_CLIST (tree), 0, 
			      tree->allocation.width - 32);

  gtk_main ();

  return 0;
}
