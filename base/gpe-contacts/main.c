/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <libintl.h>
#include <stdlib.h>
#include <string.h>
#include <libdm.h>
#include <unistd.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/smallbox.h>
#include <gpe/errorbox.h>
#include <gpe/gtkdatecombo.h>
#include <gpe/question.h>
#include <gpe/gtksimplemenu.h>
#include <gpe/picturebutton.h>

#include "interface.h"
#include "support.h"
#include "db.h"
#include "structure.h"
#include "proto.h"
#include "callbacks.h"

#define MY_PIXMAPS_DIR PREFIX "/share/gpe-contacts/pixmaps"
#define TAB_CONFIG_LOCAL ".contacts-tab"
#define TAB_CONFIG_GLOBAL "/etc/gpe/contacts-tab.conf"

static GtkWidget *categories_smenu;
gchar *active_chars;
GtkWidget *mainw;		// how ugly making this global
gboolean panel_config_has_changed = FALSE;
GtkListStore *list_store;
GtkWidget *list_view;

struct gpe_icon my_icons[] = {
  {"edit"},
  {"delete"},
  {"cancel"},
  {"frame", MY_PIXMAPS_DIR "/frame.png"},
  {"notebook", MY_PIXMAPS_DIR "/notebook.png"},
  {"entry", MY_PIXMAPS_DIR "/entry.png"},
  {"icon", PREFIX "/share/pixmaps/gpe-contacts.png" },
  {NULL, NULL}
};

/* type that holds tab configuration data */
typedef struct 
{
	char *label;
	char *chars;
}t_tabdef;

t_tabdef *tabdefs = NULL;
int num_tabs = 0;

/* loads tab config from file */
/* doesn't free existing structs! */
static int 
load_tab_config ()
{
  FILE *fnew;
  char *label, *chars;
  char *localfile;
	
  label = malloc(33);
  chars = malloc(33);
  num_tabs = 0;

  localfile = g_strdup_printf ("%s/%s", g_get_home_dir (), TAB_CONFIG_LOCAL);
  if (!access (localfile, R_OK))
    fnew = fopen (localfile, "r");
  else if (!access (TAB_CONFIG_GLOBAL, R_OK))
    fnew = fopen (TAB_CONFIG_GLOBAL, "r");
  else
    fnew = NULL;

  /* scans config file */  
  /* TAB=<label> <charlist> */
  if (fnew)
    {
      while (!feof (fnew) && (2 == fscanf (fnew, "TAB=%32s %32s\n", label, chars)))
	{
	  num_tabs++;
	  tabdefs = realloc(tabdefs,num_tabs*sizeof(t_tabdef));
	  tabdefs[num_tabs-1].label = g_strdup (label);
	  tabdefs[num_tabs-1].chars = g_strdup (chars);
	}
      fclose (fnew);
    }
  else  // defaults
    {
      tabdefs = g_malloc (7*sizeof (t_tabdef));
      tabdefs[0].label = g_strdup ("abcd");
      tabdefs[0].chars = tabdefs[0].label;
      tabdefs[1].label = g_strdup ("efgh");
      tabdefs[1].chars = tabdefs[1].label;
      tabdefs[2].label = g_strdup ("ijkl");
      tabdefs[2].chars = tabdefs[2].label;
      tabdefs[3].label = g_strdup ("mnop");
      tabdefs[3].chars = tabdefs[3].label;
      tabdefs[4].label = g_strdup ("qrstu");
      tabdefs[4].chars = tabdefs[4].label;
      tabdefs[5].label = g_strdup ("vwxyz");
      tabdefs[5].chars = tabdefs[5].label;
      tabdefs[6].label = g_strdup ("other");
      tabdefs[6].chars = g_strdup ("!abcdefghijklmnopqrstuvwxyz");
      num_tabs = 7;
    }

  free (label);
  free (chars);
  free (localfile);
  active_chars = tabdefs[0].chars;
  return num_tabs;
}

void
load_panel_config ()
{
  gchar **list;
  gint count, i;
  GtkWidget *lleft, *lright, *table, *container;

  if (panel_config_has_changed)
    {
      panel_config_has_changed = FALSE;
      table = lookup_widget (mainw, "tabDetail");
      container = lookup_widget (mainw, "pDetail");
      gtk_container_remove (GTK_CONTAINER (container), table);
      //gtk_widget_unref(table);
      gtk_widget_destroy (table);
      table = gtk_table_new (1, 2, FALSE);
      gtk_widget_set_name (table, "tabDetail");
      gtk_widget_ref (table);
      gtk_object_set_data_full (GTK_OBJECT (mainw), "tabDetail", table,
				(GtkDestroyNotify) gtk_widget_unref);
      gtk_widget_show (table);
      gtk_container_add (GTK_CONTAINER (container), table);
      gtk_table_set_col_spacings (GTK_TABLE (table), 6);
    }
  else
    table = lookup_widget (mainw, "tabDetail");
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

GtkWidget *
create_pageSetup ()
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
  GtkWidget *clist8;
  GtkWidget *label86;
  GtkWidget *label87;
  GtkWidget *frame3;

  gint tagcount, i;
  gchar **taglist;
  gchar *strvec[2];

  wSetup = mainw;

  /*
     glade generated stuff starts here
     this is the container, child in tab page     
   */
  vbox9 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox9, "vbox9");
  gtk_widget_ref (vbox9);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "vbox9", vbox9,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox9);

  frame2 = gtk_frame_new (_("Detail panel"));
  gtk_widget_set_name (frame2, "frame2");
  gtk_widget_ref (frame2);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "frame2", frame2,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame2);
  gtk_box_pack_start (GTK_BOX (vbox9), frame2, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_NONE);

  vbox10 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox10, "vbox10");
  gtk_widget_ref (vbox10);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "vbox10", vbox10,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox10);
  gtk_container_add (GTK_CONTAINER (frame2), vbox10);

  hbox4 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox4, "hbox4");
  gtk_widget_ref (hbox4);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "hbox4", hbox4,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox4);
  gtk_box_pack_start (GTK_BOX (vbox10), hbox4, FALSE, TRUE, 0);

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
  gtk_widget_set_name (bDetAdd, "bDetAdd");
  gtk_widget_ref (bDetAdd);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "bDetAdd", bDetAdd,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (bDetAdd);
  gtk_box_pack_start (GTK_BOX (hbox4), bDetAdd, FALSE, FALSE, 0);

  bDetRemove = gtk_button_new_with_label (_("Remove"));
  gtk_widget_set_name (bDetRemove, "bDetRemove");
  gtk_widget_ref (bDetRemove);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "bDetRemove", bDetRemove,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (bDetRemove);
  gtk_box_pack_start (GTK_BOX (hbox4), bDetRemove, FALSE, FALSE, 0);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow1, "scrolledwindow1");
  gtk_widget_ref (scrolledwindow1);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "scrolledwindow1",
			    scrolledwindow1,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (vbox10), scrolledwindow1, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  clist8 = gtk_clist_new (2);
  gtk_widget_set_name (clist8, "clist8");
  gtk_widget_ref (clist8);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "clist8", clist8,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (clist8);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), clist8);
  gtk_clist_set_column_width (GTK_CLIST (clist8), 0, 126);
  gtk_clist_set_column_width (GTK_CLIST (clist8), 1, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist8));

  label86 = gtk_label_new (_("Data Field"));
  gtk_widget_set_name (label86, "label86");
  gtk_widget_ref (label86);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "label86", label86,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label86);
  gtk_clist_set_column_widget (GTK_CLIST (clist8), 0, label86);

  label87 = gtk_label_new (_("Label"));
  gtk_widget_set_name (label87, "label87");
  gtk_widget_ref (label87);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "label87", label87,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label87);
  gtk_clist_set_column_widget (GTK_CLIST (clist8), 1, label87);

  frame3 = gtk_frame_new (_("Pages"));
  gtk_widget_set_name (frame3, "frame3");
  gtk_widget_ref (frame3);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "frame3", frame3,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame3);
  gtk_box_pack_start (GTK_BOX (vbox9), frame3, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame3), GTK_SHADOW_NONE);

  gtk_signal_connect (GTK_OBJECT (bDetAdd), "clicked",
		      GTK_SIGNAL_FUNC (on_bDetAdd_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (bDetRemove), "clicked",
		      GTK_SIGNAL_FUNC (on_bDetRemove_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (clist8), "select_row",
		      GTK_SIGNAL_FUNC (on_clist8_select_row), NULL);
  gtk_signal_connect (GTK_OBJECT (clist8), "unselect_row",
		      GTK_SIGNAL_FUNC (on_clist8_unselect_row), NULL);


  /* handcoded stuff */

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
  for (i = 0; i < tagcount; i++)
    {
      strvec[1] = (taglist[2 * i + 2]);
      strvec[0] = (taglist[2 * i + 3]);
      gtk_clist_append (GTK_CLIST (clist8), strvec);
    }
  db_free_result (taglist);
  return vbox9;
}


static void
update_categories (void)
{
  GSList *categories = db_get_categories (), *iter;
  GtkWidget *menu = categories_smenu;

  gtk_simple_menu_flush (GTK_SIMPLE_MENU (menu));

  gtk_simple_menu_append_item (GTK_SIMPLE_MENU (menu), _("All categories"));

  for (iter = categories; iter; iter = iter->next)
    {
      struct category *c = iter->data;
      gtk_simple_menu_append_item (GTK_SIMPLE_MENU (menu), c->name);
    }

  for (iter = categories; iter; iter = iter->next)
    {
      struct category *c = iter->data;
      g_free (c->name);
      g_free (c);
    }

  g_slist_free (categories);
}

static void
store_special_fields (GtkWidget * edit, struct person *p)
{
  GSList *l, *bl;
  GtkWidget *w = lookup_widget (edit, "datecombo");
  struct tag_value *v = p ? db_find_tag (p, "BIRTHDAY") : NULL;
  if (v && v->value)
    {
      guint year, month, day;
      sscanf (v->value, "%04d%02d%02d", &year, &month, &day);
      gtk_date_combo_set_date (GTK_DATE_COMBO (w), year, month, day);
    }
  else
    gtk_date_combo_clear (GTK_DATE_COMBO (w));

  if (p)
    {
      bl = gtk_object_get_data (GTK_OBJECT (edit), "category-widgets");
      if (bl)
	{
	  for (l = p->data; l; l = l->next)
	    {
	      v = l->data;
	      if (!strcasecmp (v->tag, "CATEGORY") && v->value)
		{
		  guint c = atoi (v->value);
		  GSList *i;
		  for (i = bl; i; i = i->next)
		    {
		      GtkWidget *w = i->data;
		      if ((guint)
			  gtk_object_get_data (GTK_OBJECT (w),
					       "category") == c)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
						      TRUE);
		    }
		}
	    }
	}
    }
}

void
edit_person (struct person *p)
{
  GtkWidget *w = edit_window ();
  GtkWidget *entry = lookup_widget (w, "name_entry");
  if (p)
    {
      GSList *tags = gtk_object_get_data (GTK_OBJECT (w), "tag-widgets");
      GSList *iter;
      for (iter = tags; iter; iter = iter->next)
	{
	  GtkWidget *w = iter->data;
	  gchar *tag = gtk_object_get_data (GTK_OBJECT (w), "db-tag");
	  struct tag_value *v = db_find_tag (p, tag);
	  guint pos = 0;
	  if (v && v->value)
	    {
	      if (GTK_IS_EDITABLE (w))
		gtk_editable_insert_text (GTK_EDITABLE (w), v->value,
					  strlen (v->value), &pos);
	      else
		gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (w)), 
					  v->value, -1);
	    }
	}
      gtk_object_set_data (GTK_OBJECT (w), "person", p);
    }
  store_special_fields (w, p);
  gtk_widget_show_all (w);
  gtk_widget_grab_focus (entry);
}

static void
new_contact (GtkWidget * widget, gpointer d)
{
  edit_person (NULL);
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

static void
new_category (GtkWidget * w, gpointer p)
{
  gchar *name = smallbox (_("New Category"), _("Name"), "");
  if (name && name[0])
    {
      gchar *line_info[1];
      guint id;
      line_info[0] = name;
      if (db_insert_category (name, &id))
	gtk_clist_append (GTK_CLIST (p), line_info);
    }

  update_categories ();
}

static void
delete_category (GtkWidget * w, gpointer clist)
{
  if (GTK_CLIST (clist)->selection)
    {
      guint row = (guint) (GTK_CLIST (clist)->selection->data);
      guint id = (guint) gtk_clist_get_row_data (GTK_CLIST (clist), row);
      if (gpe_question_ask (_("Really delete this category?"), _("Confirm"), 
			    "question", _("Delete"), "delete", _("Cancel"),
			    "cancel", NULL) == 0)
	{
	  if (db_delete_category (id))
	    {
	      gtk_clist_remove (GTK_CLIST (clist), row);
	      update_categories ();
	    }
	}
    }
}

static GtkWidget *
config_categories_box (void)
{
  GtkWidget *box = gtk_vbox_new (FALSE, 0);
  GtkWidget *clist = gtk_clist_new (1);
  GtkWidget *toolbar;
  GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);
  GSList *categories;

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			       GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

  gtk_widget_show (toolbar);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_NEW,
			    _("New category"), _("Tap here to create a category."),
			    G_CALLBACK (new_category), clist, -1);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_DELETE,
			    _("Delete category"), _("Tap here to delete the selected category."),
			    G_CALLBACK (delete_category), clist, -1);

  categories = db_get_categories ();
  if (categories)
    {
      GSList *iter;
      guint row = 0;
      
      for (iter = categories; iter; iter = iter->next)
	{
	  gchar *line_info[1];
	  struct category *c = iter->data;
	  line_info[0] = c->name;
	  gtk_clist_append (GTK_CLIST (clist), line_info);
	  gtk_clist_set_row_data (GTK_CLIST (clist), row, (gpointer) c->id);
	  g_free (c->name);
	  g_free (c);
	  row++;
	}

      g_slist_free (categories);
    }

  gtk_container_add (GTK_CONTAINER (scrolled), clist);
  gtk_widget_show (clist);
  gtk_widget_show (scrolled);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  gtk_box_pack_start (GTK_BOX (box), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), scrolled, TRUE, TRUE, 0);

  gtk_widget_show (box);
  return box;
}

static void
configure (GtkWidget * widget, gpointer d)
{
  GtkWidget *window = gtk_dialog_new ();
  GtkWidget *notebook = gtk_notebook_new ();
  GtkWidget *editlabel = gtk_label_new (_("Editing Layout"));
  GtkWidget *editbox = edit_structure ();
  GtkWidget *categorieslabel = gtk_label_new (_("Categories"));
  GtkWidget *categoriesbox = config_categories_box ();
  GtkWidget *configlabel = gtk_label_new (_("Setup"));
  GtkWidget *configbox = create_pageSetup ();
  GtkWidget *ok_button = gtk_button_new_from_stock (GTK_STOCK_OK);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), editbox, editlabel);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    categoriesbox, categorieslabel);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), configbox, configlabel);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->vbox), notebook);
  
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), ok_button, FALSE, FALSE, 0);

  gtk_window_set_default_size (GTK_WINDOW (window), 240, 300);

  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (on_setup_destroy), NULL);

  g_signal_connect_swapped (G_OBJECT (ok_button), "clicked",
			    G_CALLBACK (gtk_widget_destroy), window);

  gtk_window_set_title (GTK_WINDOW (window), _("Contacts: Configuration"));
  gpe_set_window_icon (window, "icon");

  libdm_mark_window (window);

  gtk_widget_show_all (window);
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
selection_made (GtkTreeSelection *sel)
{
  GtkTreeIter iter;
  guint id;
  struct person *p;
  GtkTreeModel *model;

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, 1, &id, -1);

      p = db_get_by_uid (id);
    
      show_details (p);

      discard_person (p);
    }
}

void
update_display (void)
{
  GSList *items = db_get_entries_alpha (active_chars), *iter;

  gtk_list_store_clear (list_store);

  for (iter = items; iter; iter = iter->next)
    {
      struct person *p = iter->data;
      GtkTreeIter iter;

      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter, 0, p->name, 1, p->id, -1);

      discard_person (p);
    }
  
  g_slist_free (items);
}

static void
main_view_switch_page (GtkNotebook * notebook,
		       GtkNotebookPage * page,
		       gint page_num, gpointer user_data)
{
  active_chars = tabdefs[page_num].chars;
  update_display ();
}

static gboolean
match_for_search (struct person *p, const gchar *text, struct category *cat)
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
  guint category = gtk_simple_menu_get_result (GTK_SIMPLE_MENU (categories_smenu));
  GSList *all_entries = db_get_entries (), *iter;
  struct category *c = NULL;

  if (category)
    {
      GSList *l = db_get_categories ();
      GSList *ll = g_slist_nth (l, category - 1);

      if (ll)
	c = ll->data;

      g_slist_free (l);
    }

  all_entries = g_slist_sort (all_entries, sort_entries);

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

static GtkWidget*
create_main (void)
{
  GtkWidget *main_window;
  GtkWidget *vbox1;
  GtkWidget *nbList;
  GtkWidget *empty_notebook_page;
  GtkWidget *label46a;
  GtkWidget *hbox3;
  GtkWidget *label83, *label84;
  GtkWidget *entry1;
  GtkWidget *pDetail;
  GtkWidget *tabDetail;
  GtkWidget *toolbar, *pw;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *tree_sel;
  GtkWidget *scrolled_window;
  GtkWidget *go_button;
  int i;

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

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Edit"), 
			   _("Edit contact"), _("Tap here to edit the selected contact."),
			   pw, (GtkSignalFunc) edit_contact, NULL);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_DELETE,
			    _("Delete contact"), _("Tap here to delete the selected contact."),
			    G_CALLBACK (delete_contact), NULL, -1);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_PROPERTIES,
			    _("Properties"), _("Tap here to configure the program."),
			    G_CALLBACK (configure), NULL, -1);
				
  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_QUIT,
			    _("Close"), _("Tap here to exit gpe-contacts."),
			    G_CALLBACK (gtk_main_quit), NULL, -1);
				
  nbList = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (vbox1), nbList, FALSE, FALSE, 0);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (nbList), FALSE);
  gtk_notebook_set_tab_hborder (GTK_NOTEBOOK (nbList), 1);
  gtk_notebook_set_tab_vborder (GTK_NOTEBOOK (nbList), 0);
  gtk_notebook_set_scrollable(GTK_NOTEBOOK (nbList), TRUE);

  load_tab_config();

  for (i=0;i<num_tabs;i++)
  {
     empty_notebook_page = gtk_vbox_new (FALSE, 0);
     label46a = gtk_label_new(tabdefs[i].label);
     gtk_widget_show (label46a);
     gtk_notebook_append_page(GTK_NOTEBOOK(nbList),empty_notebook_page,label46a);	  
  }
  
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), 
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  list_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list_view), FALSE);
  tree_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
  
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
  gtk_box_pack_start (GTK_BOX (hbox3), label84, FALSE, FALSE, 0);

  categories_smenu = gtk_simple_menu_new ();
  gtk_box_pack_start (GTK_BOX (hbox3), categories_smenu, TRUE, TRUE, 0);

  go_button = gpe_button_new_from_stock (GTK_STOCK_OK, GPE_BUTTON_TYPE_ICON);
  gtk_box_pack_start (GTK_BOX (hbox3), go_button, FALSE, FALSE, 0);  
  g_signal_connect (G_OBJECT (go_button), "clicked", G_CALLBACK (do_search), entry1);
  g_signal_connect (G_OBJECT (entry1), "activate", G_CALLBACK (do_search), entry1);

  pDetail = gtk_frame_new (_("Contact"));
  gtk_box_pack_start (GTK_BOX (vbox1), pDetail, FALSE, TRUE, 0);

  tabDetail = gtk_table_new (1, 2, FALSE);
  gtk_container_add (GTK_CONTAINER (pDetail), tabDetail);
  gtk_table_set_col_spacings (GTK_TABLE (tabDetail), 6);
  g_object_set_data (G_OBJECT (main_window), "tabDetail", tabDetail);
  g_object_set_data (G_OBJECT (main_window), "entry", entry1);

  g_signal_connect (G_OBJECT (nbList), "switch_page",
		    G_CALLBACK (main_view_switch_page), NULL);

  g_signal_connect (G_OBJECT (tree_sel), "changed",
		    G_CALLBACK (selection_made), NULL);

  libdm_mark_window (main_window);

  return main_window;
}

int
main (int argc, char *argv[])
{
#ifdef ENABLE_NLS
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
#endif
	
  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  if (db_open ())
    exit (1);

  libdm_init ();

  load_well_known_tags ();

  list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);

  mainw = create_main ();
  update_categories ();
  update_display ();
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
