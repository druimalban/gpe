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

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/smallbox.h>
#include <gpe/errorbox.h>
#include <gpe/render.h>
#include <gpe/gtkdatecombo.h>
#include <gpe/question.h>
#include <gpe/gtksimplemenu.h>

#include "interface.h"
#include "support.h"
#include "db.h"
#include "structure.h"
#include "proto.h"
#include "callbacks.h"

#define MY_PIXMAPS_DIR PREFIX "/share/gpe-contacts/pixmaps"

static GtkWidget *categories_smenu;
GtkWidget *clist;
gchar *active_chars;
GtkWidget *mainw;		// how ugly making this global
gboolean panel_config_has_changed = FALSE;

struct gpe_icon my_icons[] = {
  {"delete"},
  {"new"},
  {"save"},
  {"cancel"},
  {"edit"},
  {"properties"},
  {"frame", MY_PIXMAPS_DIR "/frame.png"},
  {"notebook", MY_PIXMAPS_DIR "/notebook.png"},
  {"entry", MY_PIXMAPS_DIR "/entry.png"},
  {"icon", PREFIX "/share/pixmaps/gpe-contacts.png" },
  {NULL, NULL}
};



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
  gtk_widget_show (w);
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
  if (GTK_CLIST (clist)->selection)
    {
      guint row = (guint) (GTK_CLIST (clist)->selection->data);
      guint uid = (guint) gtk_clist_get_row_data (GTK_CLIST (clist), row);
      struct person *p = db_get_by_uid (uid);
      edit_person (p);
    }
}

static void
delete_contact (GtkWidget * widget, gpointer d)
{
  if (GTK_CLIST (clist)->selection)
    {
      guint row = (guint) (GTK_CLIST (clist)->selection->data);
      guint uid = (guint) gtk_clist_get_row_data (GTK_CLIST (clist), row);
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
  GtkWidget *pw;
  GSList *categories;

#if GTK_MAJOR_VERSION < 2
  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_SPACE_LINE);
#else
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			       GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
#endif

  gtk_widget_show (toolbar);

  pw = gpe_render_icon (NULL, gpe_find_icon ("new"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("New"), 
			   _("New category"), 
			   _("Tap here to create a new category."), pw,
			   (GtkSignalFunc) new_category, clist);

  pw = gpe_render_icon (NULL, gpe_find_icon ("delete"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Delete"), 
			   _("Delete category"),
			   _("Tap here to delete the selected category."), pw, 
			   (GtkSignalFunc) delete_category, clist);

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
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *notebook = gtk_notebook_new ();
  GtkWidget *editlabel = gtk_label_new (_("Editing Layout"));
  GtkWidget *editbox = edit_structure ();
  GtkWidget *categorieslabel = gtk_label_new (_("Categories"));
  GtkWidget *categoriesbox = config_categories_box ();
  GtkWidget *configlabel = gtk_label_new (_("Setup"));
  GtkWidget *configbox = create_pageSetup ();

  gtk_widget_show (notebook);
  gtk_widget_show (editlabel);
  gtk_widget_show (editbox);
  gtk_widget_show (categorieslabel);
  gtk_widget_show (categoriesbox);
  gtk_widget_show (configlabel);
  gtk_widget_show (configbox);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), editbox, editlabel);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    categoriesbox, categorieslabel);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), configbox, configlabel);

  gtk_container_add (GTK_CONTAINER (window), notebook);

  gtk_widget_set_usize (window, 240, 300);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (on_setup_destroy), NULL);

  gtk_window_set_title (GTK_WINDOW (window), _("Contacts: Configuration"));
  gpe_set_window_icon (window, "icon");

  gtk_widget_show (window);
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

  if (!clist)
    return -1;
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
selection_made (GtkWidget * clist, gint row, gint column,
		GdkEventButton * event, GtkWidget * widget)
{
  guint id;
  struct person *p;

  id = (guint) gtk_clist_get_row_data (GTK_CLIST (clist), row);
  p = db_get_by_uid (id);
  show_details (p);
    
  if (event->type == GDK_2BUTTON_PRESS)
    edit_person (p);
  else
    discard_person (p);
}

void
update_display (void)
{
  GSList *items = db_get_entries_alpha (active_chars), *iter;

  if (!GTK_IS_CLIST (clist))
    return;

  gtk_clist_freeze (GTK_CLIST (clist));
  gtk_clist_clear (GTK_CLIST (clist));

  for (iter = items; iter; iter = iter->next)
    {
      struct person *p = iter->data;
      gchar *text[2];
      int row;
      text[0] = p->name;
      text[1] = NULL;
      row = gtk_clist_append (GTK_CLIST (clist), text);
      gtk_clist_set_row_data (GTK_CLIST (clist), row, (gpointer) p->id);
      discard_person (p);
    }
  
  g_slist_free (items);
  gtk_clist_thaw (GTK_CLIST (clist));
}

int
main (int argc, char *argv[])
{
  GtkWidget *toolbar;
  GtkWidget *pw;
  GtkWidget *vbox1;
  GtkWidget *hbox1;
  GtkWidget *smenu;

#ifdef ENABLE_NLS
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
#endif
	
  active_chars = malloc (5 * sizeof (gchar));
  sprintf (active_chars, "ABCD");

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  if (db_open ())
    exit (1);

  load_well_known_tags ();

  mainw = create_main ();
  smenu = gtk_simple_menu_new ();
  hbox1 = lookup_widget (GTK_WIDGET (mainw), "hbox3");
  gtk_box_pack_start (GTK_BOX (hbox1), smenu, TRUE, TRUE, 0);
  gtk_widget_show (smenu);
  categories_smenu = smenu;
  update_categories ();
  clist = lookup_widget (GTK_WIDGET (mainw), "clist1");
  update_display ();
  show_details (NULL);
  gtk_signal_connect (GTK_OBJECT (clist), "select_row",
		      GTK_SIGNAL_FUNC (selection_made), NULL);

  gtk_signal_connect (GTK_OBJECT (mainw), "destroy", gtk_main_quit, NULL);

  vbox1 = lookup_widget (mainw, "vbox1");
#if GTK_MAJOR_VERSION < 2
  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
#else
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			       GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
#endif
  gtk_box_pack_start (GTK_BOX (vbox1), toolbar, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (vbox1), toolbar, 0);
  gtk_widget_show (toolbar);

  pw = gpe_render_icon (mainw->style, gpe_find_icon ("new"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("New Contact"), 
			   _("New Contact"), _("New Contact"),
			   pw, (GtkSignalFunc) new_contact, NULL);

  pw = gpe_render_icon (mainw->style, gpe_find_icon ("edit"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Edit Contact"), 
			   _("Edit Contact"), _("Edit Contact"),
			   pw, (GtkSignalFunc) edit_contact, NULL);

  pw = gpe_render_icon (mainw->style, gpe_find_icon ("delete"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Delete Contact"), 
			   _("Delete Contact"), _("Delete Contact"), 
			   pw, (GtkSignalFunc) delete_contact, NULL);

  pw = gpe_render_icon (mainw->style, gpe_find_icon ("properties"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Configure"), 
			   _("Configure"), _("Configure"),
			   pw, (GtkSignalFunc) configure, NULL);

  load_structure ();

  // load detail panel config
  load_panel_config ();

  gpe_set_window_icon (mainw, "icon");
  gtk_widget_show (mainw);

  gtk_main ();
  return 0;
}
