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

#include "support.h"
#include "db.h"
#include "structure.h"
#include "proto.h"

#define MY_PIXMAPS_DIR PREFIX "/share/pixmaps/"
#define TAB_CONFIG_LOCAL ".contacts-tab"
#define TAB_CONFIG_GLOBAL "/etc/gpe/contacts-tab.conf"

GtkWidget *categories_smenu;
GtkWidget *mainw;
GtkListStore *list_store;
GtkWidget *list_view;
GtkWidget *search_entry;

struct gpe_icon my_icons[] = {
  {"edit"},
  {"delete"},
  {"cancel"},
  {"frame", MY_PIXMAPS_DIR "frame.png"},
  {"notebook", MY_PIXMAPS_DIR "notebook.png"},
  {"entry", MY_PIXMAPS_DIR "entry.png"},
  {"export", MY_PIXMAPS_DIR "export.png"},
  {"icon", MY_PIXMAPS_DIR "gpe-contacts.png" },
  {NULL, NULL}
};

void
load_panel_config (void)
{
  gchar **list;
  gint count, i;
  GtkWidget *lleft, *lright, *table;
  GList *wlist, *iter;

  table = lookup_widget (mainw, "tabDetail");

  wlist = gtk_container_get_children (GTK_CONTAINER (table));

  for (iter = wlist; iter; iter = iter->next)
    gtk_container_remove (GTK_CONTAINER (table), GTK_WIDGET (iter->data));

  g_list_free (wlist);

  count = db_get_config_values (CONFIG_PANEL, &list);
  for (i = 0; i < count; i++)
    {
      lleft = gtk_label_new (list[2 * i + 2]);
      lright = gtk_label_new ("");
      gtk_misc_set_alignment (GTK_MISC (lleft), 0.0, 0.5);
      gtk_misc_set_alignment (GTK_MISC (lright), 0.0, 0.5);
      gtk_widget_show (lleft);
      gtk_widget_show (lright);
      gtk_table_resize (GTK_TABLE (table), i + 1, 2);
      gtk_table_attach (GTK_TABLE (table), lleft, 0, 1, i, i + 1, GTK_FILL,
			GTK_FILL, 2, 2);
      gtk_table_attach (GTK_TABLE (table), lright, 1, 2, i, i + 1, GTK_FILL,
			GTK_FILL, 2, 2);
    }

  db_free_result (list);
}

static void
update_categories (void)
{
  GSList *categories = gpe_pim_categories_list (), *iter;
  GtkWidget *menu = categories_smenu;

  gtk_simple_menu_flush (GTK_SIMPLE_MENU (menu));

  gtk_simple_menu_append_item (GTK_SIMPLE_MENU (menu), _("All categories"));

  for (iter = categories; iter; iter = iter->next)
    {
      struct gpe_pim_category *c = iter->data;
      gtk_simple_menu_append_item (GTK_SIMPLE_MENU (menu), c->name);
    }

  g_slist_free (categories);
}

static void
export_contact (GtkWidget * widget, gpointer d)
{
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
  GtkTreeIter iter;
  GtkTreeModel *model;
  char *cmd;
  GtkWidget *dlg;
	
  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      guint uid = -1;
      gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 1, &uid, -1);
	  if (uid >= 0)
      {
         cmd = g_strdup_printf("/usr/bin/vcard-export %i > /tmp/vcard.vcf",uid);
         if (system(cmd))
           dlg = gtk_message_dialog_new (GTK_WINDOW(mainw),
              GTK_DIALOG_DESTROY_WITH_PARENT,GTK_MESSAGE_ERROR,
              GTK_BUTTONS_CLOSE, _("Export of VCARD failed."));
         else
           dlg = gtk_message_dialog_new (GTK_WINDOW(mainw),
              GTK_DIALOG_DESTROY_WITH_PARENT,GTK_MESSAGE_INFO, 
              GTK_BUTTONS_CLOSE, _("VCARD exported to\n/tmp/vcard.vcf."));
        gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
        free(cmd);
      }		  
    }
}

static void
new_contact (GtkWidget * widget, gpointer d)
{
  struct person *p;
  p = new_person ();
  edit_person (p);
}

static void
edit_contact (GtkWidget * widget, gpointer d)
{
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
  GtkTreeIter iter;
  GtkTreeModel *model;

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      guint uid;
      struct person *p;
      gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 1, &uid, -1);
      p = db_get_by_uid (uid);
      edit_person (p);
    }
}

static void
delete_contact (GtkWidget * widget, gpointer d)
{
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
  GtkTreeIter iter;
  GtkTreeModel *model;

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      guint uid;
      gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 1, &uid, -1);
      if (gpe_question_ask (_("Really delete this contact?"), _("Confirm"), 
			    "question", _("Delete"), "delete", _("Cancel"),
			    "cancel", NULL) == 0)
	{
	  if (db_delete_by_uid (uid))
	    update_display ();
	}
    }
}

static int
show_details (struct person *p)
{
  GtkWidget *lleft, *lright;
  GtkTable *table;
  gint i;
  gpointer pchild;
  gchar *tagname;
  struct tag_value *curtag;

  table = GTK_TABLE (lookup_widget (mainw, "tabDetail"));

  for (i = 0; i < table->nrows; i++)
    {
      pchild = g_list_nth_data (table->children, 2 * i);
      if (!pchild)
	continue;
      lright = ((GtkTableChild *) pchild)->widget;
      gtk_label_set_text (GTK_LABEL (lright), "");
    }
  if (p)
    {
      for (i = 0; i < table->nrows; i++)
	{
	  pchild = g_list_nth_data (table->children, 2 * i);
	  if (!pchild)
	    continue;
	  lright = ((GtkTableChild *) pchild)->widget;
	  pchild = g_list_nth_data (table->children, 2 * i + 1);
	  if (!pchild)
	    continue;
	  lleft = ((GtkTableChild *) pchild)->widget;
	  tagname =
	    db_get_config_tag (CONFIG_PANEL,
			       g_strdup (gtk_label_get_text
					 (GTK_LABEL (lleft))));
	  curtag = db_find_tag (p, tagname);
	  if (curtag != NULL)
	    {
	      gtk_label_set_text (GTK_LABEL (lright), curtag->value);
	    }
	  g_free (tagname);
	}
    }

  return 0;
}

void
selection_made (GtkTreeSelection *sel, GObject *o)
{
  GtkTreeIter iter;
  guint id;
  struct person *p;
  GtkTreeModel *model;
  GtkWidget *edit_button, *delete_button, *export_button;

  edit_button = g_object_get_data (o, "edit-button");
  delete_button = g_object_get_data (o, "delete-button");
  export_button = g_object_get_data (o, "export-button");

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, 1, &id, -1);

      p = db_get_by_uid (id);
    
      show_details (p);

      discard_person (p);
		
      gtk_widget_set_sensitive (edit_button, TRUE);
      gtk_widget_set_sensitive (delete_button, TRUE);
      gtk_widget_set_sensitive (export_button, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (edit_button, FALSE);
      gtk_widget_set_sensitive (delete_button, FALSE);
      gtk_widget_set_sensitive (export_button, FALSE);
    }
}

static gboolean
match_for_search (struct person *p, const gchar *text, struct gpe_pim_category *cat)
{
  gchar *lname = g_utf8_strdown (p->name, -1);

  if (strstr (lname, text) == NULL)
    {
      g_free (lname);
      return FALSE;
    }

  g_free (lname);

  if (cat)
    {
      GSList *l;
      gboolean found = FALSE;

      for (l = p->data; l; l = l->next)
	{
	  struct tag_value *v = l->data;
	  if (!strcasecmp (v->tag, "CATEGORY") && v->value)
	    {
	      guint c = atoi (v->value);
	      if (c == cat->id)
		{
		  found = TRUE;
		  break;
		}
	    }
	}

      if (!found)
	return FALSE;
    }

  return TRUE;
}

static void
do_search (GObject *obj, GtkWidget *entry)
{
  gchar *text = g_utf8_strdown (gtk_entry_get_text (GTK_ENTRY (entry)), -1);
  guint category = gtk_option_menu_get_history (GTK_OPTION_MENU (categories_smenu));
  GSList *all_entries = db_get_entries (), *iter;
  struct gpe_pim_category *c = NULL;

  if (category)
    {
      GSList *l = gpe_pim_categories_list ();
      GSList *ll = g_slist_nth (l, category - 1);

      if (ll)
	c = ll->data;

      g_slist_free (l);
    }

  all_entries = g_slist_sort (all_entries, (GCompareFunc)sort_entries);

  gtk_list_store_clear (list_store);

  for (iter = all_entries; iter; iter = iter->next)
    {
      struct person *p = iter->data;
      GtkTreeIter iter;

      if (match_for_search (p, text, c))
	{
	  gtk_list_store_append (list_store, &iter);
	  gtk_list_store_set (list_store, &iter, 0, p->name, 1, p->id, -1);
	}

      discard_person (p);
    }
  
  g_slist_free (all_entries);
  g_free (text);
}

#define SEARCH_DELAY  200

static guint search_seq;

struct search
{
  GtkWidget *widget;
  guint seq;
};

static gboolean
search_timer_func (gpointer data)
{
  struct search *s = (struct search *)data;

  if (s->seq == search_seq)
    do_search (G_OBJECT (s->widget), s->widget);

  g_free (s);

  return FALSE;
}

static void
schedule_search (GObject *obj)
{
  struct search *s;

  s = g_malloc0 (sizeof (*s));
  s->seq = ++search_seq;
  s->widget = GTK_WIDGET (obj);

  g_timeout_add (SEARCH_DELAY, search_timer_func, s);
}

void
update_display (void)
{
  do_search (G_OBJECT (search_entry), search_entry);
}

static GtkWidget*
create_main (void)
{
  GtkWidget *main_window;
  GtkWidget *vbox1;
  GtkWidget *hbox3;
  GtkWidget *label83, *label84;
  GtkWidget *entry1;
  GtkWidget *pDetail;
  GtkWidget *tabDetail;
  GtkWidget *toolbar, *pw;
  GtkWidget *b;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *tree_sel;
  GtkWidget *scrolled_window;

  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (main_window), _("Contacts"));
  gtk_window_set_default_size (GTK_WINDOW (main_window), 240, 320);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (main_window), vbox1);

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			       GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

  gtk_box_pack_start (GTK_BOX (vbox1), toolbar, FALSE, FALSE, 0);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_NEW,
			    _("New contact"), _("Tap here to add a new contact."),
			    G_CALLBACK (new_contact), NULL, -1);

  pw = gtk_image_new_from_pixbuf (gpe_find_icon_scaled ("edit", 
							gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar))));

  b = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Edit"), 
			       _("Edit contact"), _("Tap here to edit the selected contact."),
			       pw, (GtkSignalFunc) edit_contact, NULL);
  g_object_set_data (G_OBJECT (main_window), "edit-button", b);
  gtk_widget_set_sensitive (b, FALSE);

  b = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_DELETE,
				_("Delete contact"), _("Tap here to delete the selected contact."),
				G_CALLBACK (delete_contact), NULL, -1);
  g_object_set_data (G_OBJECT (main_window), "delete-button", b);
  gtk_widget_set_sensitive (b, FALSE);

  pw = gtk_image_new_from_pixbuf (gpe_find_icon_scaled ("export", 
							gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar))));
  b = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Export"),
			       _("Export"), _("Tap here to export this contact to VCARD."),
			       pw, G_CALLBACK (export_contact), NULL);
  g_object_set_data (G_OBJECT (main_window), "export-button", b);
  gtk_widget_set_sensitive (b, FALSE);
				
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_PROPERTIES,
			    _("Properties"), _("Tap here to configure the program."),
			    G_CALLBACK (configure), NULL, -1);
				
  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_QUIT,
			    _("Close"), _("Tap here to exit gpe-contacts."),
			    G_CALLBACK (gtk_main_quit), NULL, -1);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), 
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  list_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list_view), FALSE);
  tree_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));

  GTK_WIDGET_UNSET_FLAGS (list_view, GTK_CAN_FOCUS);
 
  gtk_container_add (GTK_CONTAINER (scrolled_window), list_view);
  gtk_box_pack_start (GTK_BOX (vbox1), scrolled_window, TRUE, TRUE, 0);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Contact"), renderer,
						     "text", 0, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

  hbox3 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox3, FALSE, FALSE, 0);

  label83 = gtk_label_new (_("Find:"));
  gtk_box_pack_start (GTK_BOX (hbox3), label83, FALSE, FALSE, 0);

  entry1 = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox3), entry1, TRUE, TRUE, 0);

  label84 = gtk_label_new (_("in"));
  gtk_box_pack_start (GTK_BOX (hbox3), label84, FALSE, FALSE, 2);

  categories_smenu = gtk_simple_menu_new ();
  gtk_box_pack_start (GTK_BOX (hbox3), categories_smenu, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (categories_smenu), "changed", 
		    G_CALLBACK (do_search), entry1);

  g_signal_connect (G_OBJECT (entry1), "activate", G_CALLBACK (do_search), entry1);
  g_signal_connect (G_OBJECT (entry1), "changed", G_CALLBACK (schedule_search), NULL);

  search_entry = entry1;

  pDetail = gtk_frame_new (_("Contact"));
  gtk_box_pack_start (GTK_BOX (vbox1), pDetail, FALSE, TRUE, 0);

  tabDetail = gtk_table_new (1, 2, FALSE);
  gtk_container_add (GTK_CONTAINER (pDetail), tabDetail);
  gtk_table_set_col_spacings (GTK_TABLE (tabDetail), 6);
  g_object_set_data (G_OBJECT (main_window), "tabDetail", tabDetail);
  g_object_set_data (G_OBJECT (main_window), "entry", entry1);

  g_signal_connect (G_OBJECT (tree_sel), "changed",
		    G_CALLBACK (selection_made), main_window);
 
  displaymigration_mark_window (main_window);

  return main_window;
}

int
main (int argc, char *argv[])
{
#ifdef ENABLE_NLS
  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);
#endif
	
  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  if (db_open ())
    exit (1);

  displaymigration_init ();

  load_well_known_tags ();

  list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);

  mainw = create_main ();
  update_categories ();
  show_details (NULL);

  gtk_signal_connect (GTK_OBJECT (mainw), "destroy", gtk_main_quit, NULL);

  load_structure ();

  // load detail panel config
  load_panel_config ();

  gpe_set_window_icon (mainw, "icon");
  gtk_widget_show_all (mainw);
  gtk_widget_grab_focus (GTK_WIDGET (g_object_get_data (G_OBJECT (mainw), "entry")));

  gtk_main ();
  return 0;
}
