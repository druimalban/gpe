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
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/errorbox.h>
#include <gpe/pixmaps.h>
#include <gpe/gtkdatecombo.h>
#include <gpe/picturebutton.h>
#include <gpe/pim-categories-ui.h>
#include <gpe/spacing.h>

#include "support.h"
#include "structure.h"
#include "db.h"
#include "main.h"

void on_edit_cancel_clicked (GtkButton * button, gpointer user_data);
void on_edit_save_clicked (GtkButton * button, gpointer user_data);
void on_edit_bt_image_clicked (GtkWidget *image, gpointer user_data);
void on_categories_clicked (GtkButton *button, gpointer user_data);
void tv_move_cursor (GtkTextView *textview,
                     GtkMovementStep arg1,
                     gint arg2, gboolean arg3,
                     gpointer user_data);
gboolean tv_focus_in (GtkWidget *widget,
                      GdkEventFocus *event,
                      gpointer user_data);

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
          GtkWidget *l;
        
          add_tag (e->tag, w, pw);
          l = gtk_label_new (e->name);
          gtk_misc_set_alignment(GTK_MISC(l),0.0,0.5);
          gtk_table_attach (GTK_TABLE (table),
                    l,
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
          g_signal_connect(G_OBJECT(ww),"move_cursor",
            G_CALLBACK(tv_move_cursor),NULL);
          g_signal_connect(G_OBJECT(ww),"focus-in-event",
            G_CALLBACK(tv_focus_in),NULL);
        
          gtk_widget_set_usize (GTK_WIDGET (ww), -1, 64);
          if (e->name)
            {
              w = gtk_frame_new (e->name);
              gtk_container_add (GTK_CONTAINER (w), ww);
              gtk_container_set_border_width (GTK_CONTAINER (w), gpe_get_border());
            }
          else
            w = ww;
          gtk_box_pack_start (GTK_BOX (vbox), w, TRUE, TRUE, 0);
          add_tag (e->tag, ww, pw);
        break;
      
        case ITEM_SINGLE_LINE:
          singles = g_slist_append (singles, e);
        break;
      
        case ITEM_DATE:
        {
          GtkWidget *l = gtk_label_new (e->name);
          GtkWidget *hbox, *esched, *datecombo;
          pop_singles (vbox, singles, pw);
          singles = NULL;
          
          hbox = gtk_hbox_new (FALSE, 0);
          gtk_misc_set_alignment (GTK_MISC (l), 0.0, 0.5);
          gtk_box_pack_start (GTK_BOX (hbox), l, TRUE, TRUE, 0);

          datecombo = gtk_date_combo_new ();
          gtk_box_pack_start (GTK_BOX (hbox), datecombo, TRUE, TRUE, 0);

          esched = gtk_check_button_new_with_label (_("Schedule"));
          gtk_widget_set_sensitive (esched, FALSE);
          gtk_box_pack_start (GTK_BOX (hbox), esched, TRUE, TRUE, 0);
          gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, gpe_get_boxspacing());
          add_tag (e->tag, datecombo, pw);
        }
        break;
        case ITEM_IMAGE:
        {
          GtkWidget *l = gtk_label_new (e->name);
          GtkWidget *hbox, *image, *btn;
          pop_singles (vbox, singles, pw);
          singles = NULL;
          
          hbox = gtk_hbox_new (FALSE, 0);
          gtk_misc_set_alignment (GTK_MISC (l), 0.0, 0.5);
          gtk_box_pack_start (GTK_BOX (hbox), l, TRUE, TRUE, 0);

          btn = gtk_button_new();
          image = gtk_image_new();
          gtk_container_add(GTK_CONTAINER(btn),image);
          gtk_box_pack_start (GTK_BOX (hbox), btn, TRUE, TRUE, 0);
          gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 
            gpe_get_boxspacing());
          g_signal_connect (G_OBJECT (btn), "clicked",
		    G_CALLBACK (on_edit_bt_image_clicked), image);
          
          add_tag (e->tag, image, pw);
        }
        default:
          /* just ignore */
        break;
      }
    }

  pop_singles (vbox, singles, pw);
  singles = NULL;
}

static gboolean
action_area_key_press_event (GtkWidget *widget, GdkEventKey *k, GtkWidget *notebook)
{
  if (k->keyval == GDK_Down) 
  {
    gtk_widget_grab_focus(notebook);
    return TRUE;
  }
  return FALSE;
}

static gboolean
notebook2_key_press_event (GtkWidget *widget, GdkEventKey *k, GtkWidget *edit_save)
{
  if (k->keyval == GDK_Up) 
  {
    if (GTK_WIDGET_HAS_FOCUS(widget))
    {
        gtk_widget_grab_focus(edit_save);
        return TRUE;
    }
  }
  return FALSE;
}

static GtkWidget*
create_edit (void)
{
  GtkWidget *edit;
  GtkWidget *notebook2;
  GtkWidget *edit_cancel;
  GtkWidget *edit_save;
  GtkWidget *edit_delete;
  GtkTooltips *tooltips;
  GtkWidget *vbox, *action_area;

  tooltips = gtk_tooltips_new ();

  if (mode_large_screen)
    {
      edit = gtk_dialog_new ();
      gtk_window_set_transient_for(GTK_WINDOW(edit),GTK_WINDOW(mainw));
      action_area = GTK_DIALOG (edit)->action_area;
      vbox = GTK_DIALOG (edit)->vbox;
    }
  else
    {
      edit = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      action_area = gtk_hbox_new (FALSE, 0);
      vbox = gtk_vbox_new (FALSE, 0);
      gtk_box_pack_end (GTK_BOX (vbox), action_area, FALSE, FALSE, 0);
      gtk_container_add (GTK_CONTAINER (edit), vbox);
    }
    
  gtk_window_set_title (GTK_WINDOW (edit), _("Edit Contact"));
  gpe_set_window_icon (edit, "icon");

  notebook2 = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (vbox), notebook2, 
		      TRUE, TRUE, 0);

  edit_delete = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  edit_cancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  edit_save = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  GTK_WIDGET_SET_FLAGS (edit_save, GTK_CAN_DEFAULT);

  gtk_widget_set_sensitive (GTK_WIDGET (edit_delete), FALSE);

  gtk_box_pack_start (GTK_BOX (action_area), edit_delete, TRUE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX (action_area), edit_cancel, TRUE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX (action_area), edit_save, TRUE, FALSE, 4);

  g_signal_connect (G_OBJECT (edit_cancel), "clicked",
		    G_CALLBACK (on_edit_cancel_clicked), edit);
  g_signal_connect (G_OBJECT (edit_save), "clicked",
		    G_CALLBACK (on_edit_save_clicked), edit);
            
  g_signal_connect (G_OBJECT (action_area), "key_press_event", 
		    G_CALLBACK (action_area_key_press_event), notebook2);
  g_signal_connect (G_OBJECT (notebook2), "key_press_event", 
		    G_CALLBACK (notebook2_key_press_event), edit_save);

  g_object_set_data (G_OBJECT (edit), "tooltips", tooltips);
  g_object_set_data (G_OBJECT (edit), "notebook2", notebook2);

  gtk_window_set_default_size (GTK_WINDOW (edit), 240, 320);

  return edit;
}

GtkWidget *
edit_window (void)
{
  GtkWidget *w = create_edit ();
  GtkWidget *book = lookup_widget (w, "notebook2");
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
      
      /* add categories stuff to the last page */
      if (!page->next)
        {
          GtkWidget *cathbox, *catbutton, *catlabel;
          cathbox = gtk_hbox_new (FALSE, gpe_get_boxspacing());
          catbutton = gtk_button_new_with_label (_("Categories"));
          catlabel = gtk_label_new (NULL);
          gtk_misc_set_alignment (GTK_MISC (catlabel), 0.0, 0.5);  
          gtk_box_pack_start (GTK_BOX (cathbox), catbutton, FALSE, FALSE, gpe_get_boxspacing());
          gtk_box_pack_start (GTK_BOX (cathbox), catlabel, TRUE, TRUE, 0);
          gtk_box_pack_start (GTK_BOX (vbox), cathbox, TRUE, FALSE, gpe_get_border());
          g_signal_connect (G_OBJECT (catbutton), "clicked",
		    G_CALLBACK (on_categories_clicked), w);
          g_object_set_data (G_OBJECT (w), "categories-label", catlabel);
        }
    }

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
  gchar *str;

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
              else if (GTK_IS_DATE_COMBO(w))
                {
                  if (v->value)
                    {
                      guint year, month, day;
                      sscanf (v->value, "%04d%02d%02d", &year, &month, &day);
                      gtk_date_combo_set_date (GTK_DATE_COMBO (w), year, month, day);
                    }
                  else
                    gtk_date_combo_clear (GTK_DATE_COMBO (w));
                }
              else if (GTK_IS_IMAGE(w))
                {
                  g_object_set_data(G_OBJECT(w),"filename",v->value);
                  if ((v->value) && !access(v->value,R_OK))
                    gtk_image_set_from_file(GTK_IMAGE(w), v->value);
                  else
                    gtk_image_set_from_stock(GTK_IMAGE(w), 
                      GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_BUTTON);
                }
              else  
                gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (w)), 
                          v->value, -1);
            }
        }
      gtk_object_set_data (GTK_OBJECT (w), "person", p);
    }
  store_special_fields (w, p);
  gtk_widget_show_all (w);
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
      gchar buf[32];
      
      if (GTK_IS_EDITABLE (w))
        text = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
      else if (GTK_IS_DATE_COMBO(w))
        {
          GtkDateCombo *c = GTK_DATE_COMBO (w);
          if (c->set)
            {
               snprintf (buf, sizeof (buf) - 1, "%04d%02d%02d", 
                 c->year, c->month, c->day);
               buf[sizeof (buf) - 1] = 0;
              text = g_strdup(buf);
            }
          else
            text = NULL;
        }
        else if (GTK_IS_IMAGE(w))
          {
            text = g_object_get_data(G_OBJECT(w),"filename");
          }
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

  w = gpe_pim_categories_dialog (get_categories_list (p), 
        G_CALLBACK (update_categories_list), user_data);
}

void 
store_filename (GtkWidget *widget, gpointer user_data)
{
  GtkWidget *selector = GTK_WIDGET(user_data);
  GtkWidget *image = g_object_get_data(G_OBJECT(selector), "image");
  gchar *old_file;
  const gchar *selected_filename =
    gtk_file_selection_get_filename (GTK_FILE_SELECTION (selector));
  
  if (selected_filename)
    {
       old_file = g_object_get_data(G_OBJECT(image),"filename");
       g_free(old_file);
       g_object_set_data(G_OBJECT(image),"filename",g_strdup(selected_filename));
       gtk_image_set_from_file(GTK_IMAGE(image), selected_filename);
    }
}

void
on_edit_bt_image_clicked (GtkWidget *bimage, gpointer user_data)
{
  GtkWidget *filesel = gtk_file_selection_new ("Select image");

  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		      "clicked", G_CALLBACK (store_filename), filesel);

  g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		    "clicked", G_CALLBACK (gtk_widget_destroy),
		    (gpointer) filesel);

  g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
		    "clicked", G_CALLBACK (gtk_widget_destroy),
		    (gpointer) filesel);
  
  g_object_set_data(G_OBJECT(filesel),"image",user_data);
  
  gtk_widget_show_all (filesel);
}

void
on_edit_cancel_clicked (GtkButton * button, gpointer user_data)
{
  gtk_widget_destroy (GTK_WIDGET (user_data));
}

void        
tv_move_cursor (GtkTextView *textview,
                GtkMovementStep arg1,
                gint arg2,
                gboolean arg3,
                gpointer user_data)
{
  GtkTextBuffer *buf = gtk_text_view_get_buffer(textview);
  
  int cnt = (int)g_object_get_data(G_OBJECT(textview),"cnt");
  
  if (arg1 == GTK_MOVEMENT_DISPLAY_LINES)
    {
      cnt += arg2;
      if (cnt >= gtk_text_buffer_get_line_count(buf))
        {
          cnt = 0;
          gtk_widget_child_focus(gtk_widget_get_toplevel(GTK_WIDGET(textview)),
		                         GTK_DIR_DOWN);
        }
      else if (cnt < 0)
        {
          cnt = 0;
          gtk_widget_child_focus(gtk_widget_get_toplevel(GTK_WIDGET(textview)),
		                         GTK_DIR_UP);
        }  
      g_object_set_data(G_OBJECT(textview),"cnt",(void*)cnt);  
    }
}

gboolean 
tv_focus_in (GtkWidget *widget,
             GdkEventFocus *event,
             gpointer user_data)
{
  GtkTextIter iter;
  GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
  gtk_text_buffer_get_start_iter(buf,&iter);
  gtk_text_buffer_place_cursor(buf,&iter);
  return FALSE;
}
