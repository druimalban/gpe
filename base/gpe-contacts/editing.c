/*
 * Copyright (C) 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include <gpe/errorbox.h>
#include <gpe/pixmaps.h>
#include <gpe/gtkdatecombo.h>
#include <gpe/picturebutton.h>
#include <gpe/pim-categories-ui.h>

#include "support.h"
#include "structure.h"
#include "db.h"
#include "main.h"

void on_edit_cancel_clicked (GtkButton * button, gpointer user_data);
void on_edit_save_clicked (GtkButton * button, gpointer user_data);
void on_edit_bt_image_clicked (GtkButton * button, gpointer user_data);
void on_categories_clicked (GtkButton *button, gpointer user_data);

static void
add_tag (gchar *tag, GtkWidget *w, GtkWidget *pw)
{
  GSList *tags;
  g_object_set_data (G_OBJECT (w), "db-tag", tag);

  tags = gtk_object_get_data (GTK_OBJECT (pw), "tag-widgets");
  tags = g_slist_append (tags, w);
  g_object_set_data (G_OBJECT (pw), "tag-widgets", tags);
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
	  {
	    gchar *markup = g_strdup_printf ("<b>%s</b>", e->name);
	    w = gtk_label_new (NULL);
	    gtk_label_set_markup (GTK_LABEL (w), markup);
	    gtk_misc_set_alignment (GTK_MISC (w), 0, 0.5);
	    g_free (markup);
	    pop_singles (vbox, singles, pw);
	    singles = NULL;
	    gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
	    build_children (vbox, e->children, pw);
	  }
	  break;

	case ITEM_MULTI_LINE:
	  pop_singles (vbox, singles, pw);
	  singles = NULL;
	  ww = gtk_text_view_new ();
	  gtk_text_view_set_editable (GTK_TEXT_VIEW (ww), TRUE);
	  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (ww), GTK_WRAP_WORD);
	  gtk_widget_set_usize (GTK_WIDGET (ww), -1, 64);
	  if (e->name)
	    {
	      w = gtk_frame_new (e->name);
	      gtk_container_add (GTK_CONTAINER (w), ww);
	      gtk_container_set_border_width (GTK_CONTAINER (w), 2);
	    }
	  else
	    w = ww;
	  gtk_box_pack_start (GTK_BOX (vbox), w, TRUE, TRUE, 4);
	  add_tag (e->tag, ww, pw);
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

static GtkWidget*
create_edit (void)
{
  GtkWidget *edit;
  GtkWidget *notebook2;
  GtkWidget *table1;
  GtkWidget *edit_bt_name;
  GtkWidget *label19;
  GtkWidget *label20;
  GtkWidget *label22;
  GtkWidget *hbox3;
  GtkWidget *datecombo;
  GtkWidget *edit_bt_bdate;
  GtkWidget *hbox4;
  GtkWidget *edit_bt_image;
  GtkWidget *cathbox;
  GtkWidget *name_entry;
  GtkWidget *summary_entry;
  GtkWidget *label16;
  GtkWidget *edit_cancel;
  GtkWidget *edit_save;
  GtkWidget *edit_delete;
  GtkWidget *catlabel, *catbutton;
  GtkTooltips *tooltips;
  GtkWidget *topvbox;
  GtkWidget *topscrolledwindow;
  GtkWidget *vbox, *action_area;

  tooltips = gtk_tooltips_new ();

#if 0
  edit = gtk_dialog_new ();
  action_area = GTK_DIALOG (edit)->action_area;
  vbox = GTK_DIALOG (edit)->vbox;
#else
  edit = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  action_area = gtk_hbox_new (FALSE, 0);
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), action_area, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (edit), vbox);
#endif

  gtk_window_set_title (GTK_WINDOW (edit), _("Edit Contact"));
  gpe_set_window_icon (edit, "icon");

  notebook2 = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (vbox), notebook2, 
		      TRUE, TRUE, 0);

  topscrolledwindow = gtk_scrolled_window_new (NULL, NULL);

  topvbox = gtk_vbox_new (FALSE, 0);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (topscrolledwindow),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (topscrolledwindow), topvbox);

  gtk_container_add (GTK_CONTAINER (notebook2), topscrolledwindow/*topvbox*/);

  table1 = gtk_table_new (4, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (topvbox), table1, FALSE, FALSE, 2);
  gtk_table_set_row_spacings (GTK_TABLE (table1), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table1), 3);

  edit_bt_name = gtk_button_new_with_label (_("Name"));
  gtk_table_attach (GTK_TABLE (table1), edit_bt_name, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, edit_bt_name, _("more detailed name options"), NULL);

  label19 = gtk_label_new (_("Summary"));
  gtk_table_attach (GTK_TABLE (table1), label19, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label19), 0.5, 0.5);

  label20 = gtk_label_new (_("Birthday"));
  gtk_table_attach (GTK_TABLE (table1), label20, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label20), 0.5, 0.5);

  label22 = gtk_label_new (_("Image"));
  gtk_table_attach (GTK_TABLE (table1), label22, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label22), 0.5, 0.5);

  hbox3 = gtk_hbox_new (FALSE, 0);
  gtk_table_attach (GTK_TABLE (table1), hbox3, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  datecombo = gtk_date_combo_new ();
  gtk_box_pack_start (GTK_BOX (hbox3), datecombo, TRUE, TRUE, 0);
  GTK_WIDGET_UNSET_FLAGS (datecombo, GTK_CAN_FOCUS);
  GTK_WIDGET_UNSET_FLAGS (datecombo, GTK_CAN_DEFAULT);

  edit_bt_bdate = gtk_check_button_new_with_label (_("Schedule"));
  gtk_widget_set_sensitive (edit_bt_bdate, FALSE);		/* XXX */
  gtk_box_pack_start (GTK_BOX (hbox3), edit_bt_bdate, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, edit_bt_bdate, _("automatic appointment"), NULL);

  hbox4 = gtk_hbox_new (FALSE, 0);
  gtk_table_attach (GTK_TABLE (table1), hbox4, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  edit_bt_image = gtk_button_new_with_label ("");
  gtk_box_pack_start (GTK_BOX (hbox4), edit_bt_image, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, edit_bt_image, _("click to choose file"), NULL);

  cathbox = gtk_hbox_new (FALSE, 0);
  catbutton = gtk_button_new_with_label (_("Categories:"));
  catlabel = gtk_label_new (NULL);

  gtk_box_pack_start (GTK_BOX (cathbox), catbutton, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (cathbox), catlabel, TRUE, TRUE, 4);
  gtk_misc_set_alignment (GTK_MISC (catlabel), 0.0, 0.5);  
  
  gtk_box_pack_start (GTK_BOX (topvbox), cathbox, FALSE, FALSE, 2);

  name_entry = gtk_entry_new ();
  add_tag ("NAME", name_entry, edit);
  gtk_table_attach (GTK_TABLE (table1), name_entry, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  summary_entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table1), summary_entry, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label16 = gtk_label_new (_("Personal"));
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook2), 0), label16);

  edit_delete = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  edit_cancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  edit_save = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  GTK_WIDGET_SET_FLAGS (edit_save, GTK_CAN_DEFAULT);

  gtk_widget_set_sensitive (GTK_WIDGET (edit_delete), FALSE);

  gtk_box_pack_start (GTK_BOX (action_area), edit_delete, TRUE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX (action_area), edit_cancel, TRUE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX (action_area), edit_save, TRUE, FALSE, 4);

  g_signal_connect (G_OBJECT (edit_bt_image), "clicked",
		    G_CALLBACK (on_edit_bt_image_clicked), NULL);
  g_signal_connect (G_OBJECT (edit_cancel), "clicked",
		    G_CALLBACK (on_edit_cancel_clicked), edit);
  g_signal_connect (G_OBJECT (edit_save), "clicked",
		    G_CALLBACK (on_edit_save_clicked), edit);
  g_signal_connect (G_OBJECT (catbutton), "clicked",
		    G_CALLBACK (on_categories_clicked), edit);

  g_object_set_data (G_OBJECT (edit), "tooltips", tooltips);
  g_object_set_data (G_OBJECT (edit), "notebook2", notebook2);
  g_object_set_data (G_OBJECT (edit), "name_entry", name_entry);
  g_object_set_data (G_OBJECT (edit), "datecombo", datecombo);
  g_object_set_data (G_OBJECT (edit), "categories-label", catlabel);

  gtk_window_set_default_size (GTK_WINDOW (edit), 240, 320);

  return edit;
}

GtkWidget *
edit_window (void)
{
  GtkWidget *w = create_edit ();
  GtkWidget *book = lookup_widget (w, "notebook2");
  GtkWidget *displaylabel = gtk_label_new ("Display");
  GtkWidget *displayvbox = gtk_vbox_new (FALSE, 0);
  GSList *page;

  for (page = edit_pages; page; page = page->next)
    {
      edit_thing_t e = page->data;
      GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
      GtkWidget *label = gtk_label_new (e->name);
      GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);

      gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);

      build_children (vbox, e->children, w);

      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
      gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), vbox);

      gtk_notebook_append_page (GTK_NOTEBOOK (book), scrolled_window, label);
    }

#if 0
  gtk_notebook_append_page (GTK_NOTEBOOK (book), displayvbox, 
			    displaylabel);
#endif
      
  return w;
}

void
retrieve_special_fields (GtkWidget * edit, struct person *p)
{
  GSList *cl = g_object_get_data (G_OBJECT (edit), "category-widgets");
  db_delete_tag (p, "CATEGORY");
  while (cl)
    {
      GtkWidget *w = cl->data;
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
	{
	  guint c = (guint) g_object_get_data (G_OBJECT (w), "category");
	  char buf[32];
	  snprintf (buf, sizeof (buf) - 1, "%d", c);
	  buf[sizeof (buf) - 1] = 0;
	  db_set_multi_data (p, "CATEGORY", g_strdup (buf));
	}
      cl = cl->next;
    }

  db_delete_tag (p, "BIRTHDAY");
  {
    GtkDateCombo *c = GTK_DATE_COMBO (lookup_widget (edit, "datecombo"));
    if (c->set)
      {
	char buf[32];
	snprintf (buf, sizeof (buf) - 1, "%04d%02d%02d", c->year, c->month,
		  c->day);
	buf[sizeof (buf) - 1] = 0;
	db_set_data (p, "BIRTHDAY", g_strdup (buf));
      }
  }
}

static GSList *
get_categories_list (struct person *p)
{
  GSList *iter;
  GSList *list = NULL;

  iter = p->data;

  for (; iter; iter = iter->next)
    {
      struct tag_value *t = iter->data;
      if (!strcasecmp (t->tag, "category"))
	{
	  int id = atoi (t->value);

	  list = g_slist_prepend (list, (gpointer)id);
	}
    }

  return list;
}

static gchar *
build_categories_string (struct person *p)
{
  gchar *s = NULL;
  GSList *iter, *list;

  list = get_categories_list (p);

  for (iter = list; iter; iter = iter->next)
    {
      const gchar *cat;
      cat = gpe_pim_category_name ((int)iter->data);

      if (cat)
	{
	  if (s)
	    {
	      char *ns = g_strdup_printf ("%s, %s", s, cat);
	      g_free (s);
	      s = ns;
	    }
	  else
	    s = g_strdup (cat);
	}
    }

  g_slist_free (list);

  return s;
}

static void
update_categories_list (GtkWidget *ui, GSList *selected, GtkWidget *edit)
{
  struct person *p;
  gchar *str;
  GtkWidget *w;
  GSList *iter;

  p = g_object_get_data (G_OBJECT (edit), "person");

  db_delete_tag (p, "category");

  for (iter = selected; iter; iter = iter->next)
    {
      db_set_multi_data (p, "category", g_strdup_printf ("%d", (int)iter->data));
    }

  str = build_categories_string (p);
  
  w = lookup_widget (edit, "categories-label");
  gtk_label_set_text (GTK_LABEL (w), str);
  g_free (str);
}

static void
store_special_fields (GtkWidget *edit, struct person *p)
{
  GtkWidget *w;
  struct tag_value *v;
  gchar *str;

  w = lookup_widget (edit, "datecombo");
  v = p ? db_find_tag (p, "BIRTHDAY") : NULL;
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
      str = build_categories_string (p);
      if (str)
	{
	  w = lookup_widget (edit, "categories-label");
	  gtk_label_set_text (GTK_LABEL (w), str);
	  g_free (str);
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

void
on_edit_save_clicked (GtkButton * button, gpointer user_data)
{
  GtkWidget *edit = (GtkWidget *) user_data;
  GSList *tags;
  struct person *p = g_object_get_data (G_OBJECT (edit), "person");
  
  for (tags = g_object_get_data (G_OBJECT (edit), "tag-widgets");
       tags; tags = tags->next)
    {
      GtkWidget *w = tags->data;
      gchar *text, *tag;
      if (GTK_IS_EDITABLE (w))
	text = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
      else
	{
	  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w));
	  GtkTextIter start, end;
	  gtk_text_buffer_get_bounds (buf, &start, &end);
	  text = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
	}
      tag = g_object_get_data (G_OBJECT (w), "db-tag");
      db_set_data (p, tag, text);
    }

  retrieve_special_fields (edit, p);

  if (commit_person (p))
    {
      gtk_widget_destroy (edit);
      discard_person (p);
      update_display ();
    }
}

void
on_categories_clicked (GtkButton *button, gpointer user_data)
{
  struct person *p;
  GtkWidget *w;

  p = g_object_get_data (G_OBJECT (user_data), "person");

  w = gpe_pim_categories_dialog (get_categories_list (p), G_CALLBACK (update_categories_list), user_data);
}

void 
store_filename (GtkWidget * w, GtkFileSelection * selector)
{
  const gchar *selected_filename =
    gtk_file_selection_get_filename (GTK_FILE_SELECTION (selector));
}

void
on_edit_bt_image_clicked (GtkButton * button, gpointer user_data)
{
  GtkWidget *filesel = gtk_file_selection_new ("Select image");

  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		      "clicked", G_CALLBACK (store_filename), filesel);

  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		    "clicked", G_CALLBACK (gtk_widget_destroy),
		    (gpointer) filesel);

  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
		    "clicked", G_CALLBACK (gtk_widget_destroy),
		    (gpointer) filesel);

  gtk_widget_show_all (filesel);
}

void
on_edit_cancel_clicked (GtkButton * button, gpointer user_data)
{
  gtk_widget_destroy (GTK_WIDGET (user_data));
}
