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
#include <ctype.h>
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
#include "namedetail.h"

void on_edit_cancel_clicked (GtkButton * button, gpointer user_data);
void on_edit_window_closed_clicked (gpointer user_data);
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
gboolean tv_focus_out (GtkWidget *widget,
                       GdkEventFocus *event,
                       gpointer user_data);
void on_unknown_year_toggled (GtkToggleButton *togglebutton, gpointer user_data);
void on_name_clicked (GtkButton *button, gpointer user_data);



/* this is the filter for phone number edits */
static gboolean
phone_key_press_event (GtkWidget *widget, GdkEventKey *k, gpointer p)
{
  if (!strstr(" +0123456789-/()", k->string) 
      && !(k->state & GDK_CONTROL_MASK))
    {
      return TRUE;
    }
  return FALSE;
}

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
pop_singles (GtkWidget *vbox, GSList *list, GtkWidget *pw, gboolean visible)
{
  if (list)
    {
      guint l = g_slist_length (list);
      GtkWidget *table = gtk_table_new (l, gpe_get_boxspacing(), FALSE);
      guint x = 0;
    
      while (list)
        {
          GSList *next = list->next;
          edit_thing_t e = list->data;
          GtkWidget *w = gtk_entry_new ();
          GtkWidget *l;
          
          add_tag (e->tag, w, pw);
          if (strstr(e->tag,".TELEPHONE")
              || strstr(e->tag,".MOBILE") 
              || strstr(e->tag,".FAX"))
            {
               g_signal_connect (G_OBJECT (w), "key_press_event", 
		         G_CALLBACK (phone_key_press_event), NULL);
            }
          if (strcasecmp(e->tag, "NAME")) /* the name field on a button */
            {
              l = gtk_label_new (e->name);
              gtk_misc_set_alignment(GTK_MISC(l),1.0,0.5);
            }
          else
            {
              GtkWidget *b, *h;
              h = gtk_hbox_new(FALSE, gpe_get_boxspacing());
              l = gtk_label_new (e->name);
              gtk_box_pack_start(GTK_BOX(h), w, TRUE, TRUE, 0);
              GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
              b = gtk_button_new_with_label(_("Details"));
              gtk_box_pack_start(GTK_BOX(h), b, FALSE, TRUE, 0);
              g_object_set_data(G_OBJECT(b), "edit", w);
              w = h;
              g_signal_connect(G_OBJECT(b),"clicked",
                               G_CALLBACK(on_name_clicked),
                               gtk_widget_get_toplevel(pw));
            }
          gtk_table_attach (GTK_TABLE (table),
                    l,
                    0, 1, x, x + 1,
                    GTK_FILL, GTK_FILL, 0, 0);
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

      if (visible)
        gtk_widget_show_all(table);
      else
        gtk_widget_hide_all(table);
      gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 4);
    }
}

static void
build_children (GtkWidget *vbox, GSList *children, GtkWidget *pw, gboolean visible)
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
            if (visible)
              gtk_widget_show(w);
            else
              gtk_widget_hide(w);
            gtk_label_set_markup (GTK_LABEL (w), markup);
            gtk_misc_set_alignment (GTK_MISC (w), 0, 0.5);
            g_free (markup);
            pop_singles (vbox, singles, pw, visible);
            singles = NULL;
            gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, TRUE, 0);
            /* hidden group */
            build_children (vbox, e->children, pw, !e->hidden);
            if (e->hidden)
              gtk_widget_hide(w);
          }
        break;
      
        case ITEM_MULTI_LINE:
          pop_singles (vbox, singles, pw, visible);
          singles = NULL;
          ww = gtk_text_view_new ();
          if (visible)
            gtk_widget_show(ww);
          else
            gtk_widget_hide(ww);
          gtk_text_view_set_editable (GTK_TEXT_VIEW (ww), TRUE);
          gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (ww), GTK_WRAP_WORD);
          g_signal_connect(G_OBJECT(ww),"move_cursor",
            G_CALLBACK(tv_move_cursor),NULL);
          g_signal_connect(G_OBJECT(ww),"focus-in-event",
            G_CALLBACK(tv_focus_in),NULL);
          g_signal_connect(G_OBJECT(ww),"focus-out-event",
            G_CALLBACK(tv_focus_out),NULL);
        
          gtk_widget_set_usize (GTK_WIDGET (ww), -1, 64);
          if (e->name)
            {
              w = gtk_frame_new (e->name);
              if (visible)
                gtk_widget_show(w);
              else
                gtk_widget_hide(w);
              gtk_container_add (GTK_CONTAINER (w), ww);
              gtk_container_set_border_width (GTK_CONTAINER (w), gpe_get_border());
            }
          else
            w = ww;
          gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, TRUE, 0);
          add_tag (e->tag, ww, pw);
        break;
      
        case ITEM_SINGLE_LINE:
          singles = g_slist_append (singles, e);
        break;
      
        case ITEM_DATE:
        {
          GtkWidget *l = gtk_label_new (e->name);
          GtkWidget *hbox, *datecombo, *cbnoyear;
          pop_singles (vbox, singles, pw, visible);
          singles = NULL;
          
          hbox = gtk_hbox_new (FALSE, gpe_get_boxspacing());
          gtk_misc_set_alignment (GTK_MISC (l), 0.0, 0.5);
          gtk_box_pack_start (GTK_BOX (hbox), l, TRUE, TRUE, 0);

          datecombo = gtk_date_combo_new ();
          gtk_box_pack_start (GTK_BOX (hbox), datecombo, TRUE, TRUE, 0);
          gtk_date_combo_clear(GTK_DATE_COMBO(datecombo));
          cbnoyear = gtk_check_button_new_with_label (_("no year"));
          gtk_box_pack_start (GTK_BOX (hbox), cbnoyear, TRUE, TRUE, 0);
          g_signal_connect (G_OBJECT (cbnoyear), "toggled",
		    G_CALLBACK (on_unknown_year_toggled), datecombo);
          g_object_set_data(G_OBJECT(datecombo),"cbnoyear",cbnoyear);
          if (visible)
            gtk_widget_show_all(hbox);
          else
            gtk_widget_hide_all(hbox);
          gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, gpe_get_boxspacing());
          add_tag (e->tag, datecombo, pw);
        }
        break;
        case ITEM_IMAGE:
        {
          GtkWidget *l = gtk_label_new (e->name);
          GtkWidget *hbox, *image, *btn, *sw, *vp;
          pop_singles (vbox, singles, pw, visible);
          singles = NULL;
          
          hbox = gtk_hbox_new (FALSE, gpe_get_boxspacing());
          gtk_misc_set_alignment (GTK_MISC (l), 0.0, 0.5);
          gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, TRUE, 0);

          btn = gtk_button_new();
          image = gtk_image_new();
          vp = gtk_viewport_new(NULL, NULL);
          gtk_viewport_set_shadow_type(GTK_VIEWPORT(vp), GTK_SHADOW_NONE);
          sw = gtk_scrolled_window_new(NULL, NULL);
          gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), 
                                         GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
          gtk_container_add(GTK_CONTAINER(vp), btn);
          gtk_container_add(GTK_CONTAINER(sw), vp);
          gtk_container_add(GTK_CONTAINER(btn), image);
          gtk_box_pack_start (GTK_BOX (hbox), sw, TRUE, TRUE, 0);
          gtk_widget_show_all(hbox);
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

  pop_singles (vbox, singles, pw, visible);
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

static gboolean
edit_window_key_press_event (GtkWidget *widget, GdkEventKey *k, GtkWidget *user_data)
{
  if (k->keyval == GDK_Escape) 
  {
    on_edit_window_closed_clicked(GTK_WIDGET (user_data));
    return TRUE;
  }
  if (k->keyval == GDK_Return) 
  {
    if (!g_object_get_data(G_OBJECT(widget),"inmultiline"))
      {
        on_edit_save_clicked (NULL, user_data);
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
  GtkWidget *edit_delete = NULL;
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
      action_area = gtk_hbutton_box_new();
	  gtk_button_box_set_spacing(GTK_BUTTON_BOX(action_area),
		                         gpe_get_boxspacing());
	  gtk_button_box_set_layout(GTK_BUTTON_BOX(action_area),
		                        GTK_BUTTONBOX_END);
      vbox = gtk_vbox_new (FALSE, 0);
      gtk_box_pack_end (GTK_BOX (vbox), action_area, FALSE, FALSE, 0);
      gtk_container_add (GTK_CONTAINER (edit), vbox);
    }
  
  gpe_set_window_icon (edit, "icon");

  notebook2 = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (vbox), notebook2, TRUE, TRUE, 0);
  
  if (mode_landscape || mode_large_screen)
    edit_delete = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  edit_cancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  edit_save = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  GTK_WIDGET_SET_FLAGS (edit_save, GTK_CAN_DEFAULT);

  if (mode_landscape || mode_large_screen)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (edit_delete), FALSE);
      gtk_box_pack_start (GTK_BOX (action_area), edit_delete, TRUE, FALSE, 4);
    }
  gtk_box_pack_start (GTK_BOX (action_area), edit_cancel, TRUE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX (action_area), edit_save, TRUE, FALSE, 4);

  g_signal_connect (G_OBJECT (edit_cancel), "clicked",
		    G_CALLBACK (on_edit_cancel_clicked), edit);
  g_signal_connect (G_OBJECT (edit_save), "clicked",
		    G_CALLBACK (on_edit_save_clicked), edit);

  /* Call the on_edit_window_closed_clicked function when the window is destroyed,
   * otherwise the new button gets disabled */
  g_signal_connect (G_OBJECT (edit), "delete_event",
		  	G_CALLBACK (on_edit_window_closed_clicked), edit);
            
  g_signal_connect (G_OBJECT (action_area), "key_press_event", 
		    G_CALLBACK (action_area_key_press_event), notebook2);
  g_signal_connect (G_OBJECT (notebook2), "key_press_event", 
		    G_CALLBACK (notebook2_key_press_event), edit_save);
  g_signal_connect (G_OBJECT (edit), "key_press_event", 
		    G_CALLBACK (edit_window_key_press_event), edit);

  g_object_set_data (G_OBJECT (edit), "tooltips", tooltips);
  g_object_set_data (G_OBJECT (edit), "notebook2", notebook2);

  if (mode_large_screen)
    {
        gtk_window_set_default_size (GTK_WINDOW (edit), 320, 480);
    }
  else
    {
      if (mode_landscape)
        gtk_window_set_default_size (GTK_WINDOW (edit), 320, 240);
      else
        gtk_window_set_default_size (GTK_WINDOW (edit), 240, 320);
    }

  return edit;
}

GtkWidget *
edit_window (gboolean isdialog)
{
  GtkWidget *w = create_edit ();
  GtkWidget *book = lookup_widget (w, "notebook2");
  GSList *page;

  gtk_widget_show_all(w);

  for (page = edit_pages; page; page = page->next)
    {
      edit_thing_t e = page->data;
      GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
      GtkWidget *label = gtk_label_new (e->name);
      GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);

      gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);

      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
      gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), vbox);
      gtk_widget_show_all(scrolled_window);
      gtk_widget_show(label);
      gtk_notebook_append_page (GTK_NOTEBOOK (book), scrolled_window, label);
      build_children (vbox, e->children, w, TRUE);
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
          gtk_box_pack_start (GTK_BOX (vbox), cathbox, FALSE, TRUE, gpe_get_border());
          g_signal_connect (G_OBJECT (catbutton), "clicked",
		    G_CALLBACK (on_categories_clicked), w);
          g_object_set_data (G_OBJECT (w), "categories-label", catlabel);
          if (!isdialog)
            gtk_widget_show_all(cathbox);
        }
    }

  return w;
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

gchar *
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

void
edit_person (struct person *p, gchar *title, gboolean isdialog)
{
  GtkWidget *catlabel, *w = edit_window (isdialog);
  gchar *str;
  
  gtk_window_set_title (GTK_WINDOW (w), title);
  g_object_set_data(G_OBJECT(w), "isdialog", (gpointer)isdialog);  
  
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
          if (tag)
            {
               if (!strcasecmp(tag, "NAME"))
                 {
                   gtk_widget_grab_default(w);
                   gtk_widget_grab_focus(w);
                 }
            }
          if (v && v->value)
            {
              if (GTK_IS_EDITABLE (w))
                {
                  gtk_editable_insert_text (GTK_EDITABLE (w), v->value,
                          strlen (v->value), &pos);
                }
              else if (GTK_IS_DATE_COMBO(w))
                {
                  if (v->value)
                    {
                      guint year, month, day;
                      GtkToggleButton *cbnoyear;
                      
                      sscanf (v->value, "%04d%02d%02d", &year, &month, &day);
                      gtk_date_combo_set_date (GTK_DATE_COMBO (w), year, month, day);
                      if (year == 0)
                        {
                          gtk_date_combo_ignore_year(GTK_DATE_COMBO(w),TRUE);
                          cbnoyear = g_object_get_data(G_OBJECT(w),"cbnoyear");
                          gtk_toggle_button_set_active(cbnoyear,TRUE);
                        }
                    }
                  else
                    gtk_date_combo_clear (GTK_DATE_COMBO (w));
                }
              else if (GTK_IS_IMAGE(w))
                {
                  if ((v->value) && !access(v->value,R_OK)) 
                    {
                      g_object_set_data(G_OBJECT(w),"filename",v->value);
                      gtk_image_set_from_file(GTK_IMAGE(w), v->value);
                    }
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
        
      /* display categories */  
      str = build_categories_string (p);
      catlabel = lookup_widget (w, "categories-label");
      gtk_label_set_text (GTK_LABEL (catlabel), str);
      g_free (str);
    }
  gtk_widget_show (w);
}

void
update_edit (struct person *p, GtkWidget *w)
{
  GtkWidget *nameentry[5];
  int namenum = 0;
  int i;
  gchar *n1 = NULL, *n2 = NULL, *n3 = NULL, *n4 = NULL, *n5 = NULL;
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
               /* collect data for name field update */
              if (!strcasecmp(v->tag,"TITLE"))
                n1 = v->value;
              if (!strcasecmp(v->tag,"GIVEN_NAME"))
                n2 = v->value;
              if (!strcasecmp(v->tag,"MIDDLE_NAME"))
                n3 = v->value;
              if (!strcasecmp(v->tag,"FAMILY_NAME"))
                n4 = v->value;
              if (!strcasecmp(v->tag,"HONORIFIC_SUFFIX"))
                n5 = v->value;
              if (!strcasecmp(v->tag,"NAME"))
              {
                nameentry[namenum] = w;
                namenum++;
              }

              if (GTK_IS_EDITABLE (w))
                {
                  gtk_editable_delete_text(GTK_EDITABLE(w), 0, -1);
                  gtk_editable_insert_text (GTK_EDITABLE (w), v->value,
                                            strlen (v->value), &pos);
                }
              else if (GTK_IS_DATE_COMBO(w))
                {
                  if (v->value)
                    {
                      guint year, month, day;
                      GtkToggleButton *cbnoyear;
                      
                      sscanf (v->value, "%04d%02d%02d", &year, &month, &day);
                      gtk_date_combo_set_date (GTK_DATE_COMBO (w), year, month, day);
                      if (year == 0)
                        {
                          gtk_date_combo_ignore_year(GTK_DATE_COMBO(w),TRUE);
                          cbnoyear = g_object_get_data(G_OBJECT(w),"cbnoyear");
                          gtk_toggle_button_set_active(cbnoyear,TRUE);
                        }
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
              else if (GTK_IS_TEXT_VIEW(w))
                gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (w)), 
                          v->value, -1);
            }
        }
    }

  if (namenum)
    {
      gchar *ts = g_strdup(n1);
      if (n2)
        {
          n1 = g_strdup_printf("%s %s", ts, g_strstrip(n2));
          g_free(ts);
          ts = n1;
        }
      if (n3) 
        {
          n1 = g_strdup_printf("%s %s", ts, g_strstrip(n3));
          g_free(ts);
          ts = n1;
        }
      if (n4) 
        {
          n1 = g_strdup_printf("%s %s", ts, g_strstrip(n4));
          g_free(ts);
          ts = n1;
        }
      if (n5) 
        {
          n1 = g_strdup_printf("%s %s", ts, g_strstrip(n5));
          g_free(ts);
          ts = n1;
        }

      for (i=0;i<namenum;i++)
        gtk_entry_set_text(GTK_ENTRY(nameentry[i]), g_strstrip(ts));
      g_free(ts);
    }
}


void
store_name_fields(struct person *p, const char *nametext)
{
char e[4][100];
int num_elements, i;
gboolean has_title = FALSE, has_suffix = FALSE;
char *sp;
char *title = NULL;
char *first = NULL;
char *middle = NULL;
char *last = NULL;
char *suffix = NULL;
   
  if (!nametext) return;
  if (strlen(nametext) < 3) return;
    sp = g_strdup(nametext);

    /* scan first element for title */
    if (sscanf(sp, "%s", e[0]))
      {
        struct tag_value *v;
        i = 0;
        
        while (titles[i])
          {
            if (!strcasecmp(e[0], titles[i]))
              {
                has_title = TRUE;
                title = sp;
                sp[strlen(e[0])] = 0;
                sp += strlen(e[0])+1;
                break;
              }
            i++;
          }
        if (!has_title) /* find user defined title */
          {
            v = db_find_tag(p, "TITLE");
            if (v && v->value && !strcasecmp(e[0], v->value))
              {
                has_title = TRUE;
                title = sp;
                sp[strlen(v->value)] = 0;
                sp += strlen(v->value)+1;
              }
          }
      }
    
    /* strip trailing spaces */
    while (strlen(sp) && isblank(sp[0]))
      sp++;
    
    /* scan last element for suffix */
    if ((long)(suffix = strrchr(sp, ' ') + 1) > 1)
      {
        i = 0;
        while (suffixes[i])
          {
            if (!strcmp(suffix, suffixes[i]))
              {
                char *t;
                has_suffix = TRUE;
                t = strstr(sp, suffixes[i]);
                if (t)
                  {
                    t--;
                    t[0] = 0;
                  }
                break;
              }
            i++;
          }
      }
    if (!has_suffix) 
      suffix = NULL;
   
    /* count remaining elements */
    num_elements = sscanf(sp, 
                          "%100s %100s %100s %100s", 
                          e[0], e[1], e[2], e[3]);
    
    switch (num_elements)
      {
        case 0: 
        case EOF:
        break;
        case 1: 
          last = e[0];
        break;
        case 2: 
          if (strstr(sp, ","))
            {
              last = sp;
              first = strstr(sp, ",") + 1;
              while (strlen(first) && isblank(first[0]))
                first++;
              strstr(sp, ",")[0] = 0;
            }
          else
            {
              first = sp;
              last = strrchr(sp, ' ') + 1;
              while (strlen(last) && isblank(last[0]))
                last++;
              strrchr(sp, ' ')[0] = 0;
            }
        break;
        default:
          if (strstr(sp, ","))
            {
              last = sp;
              first = strstr(sp, ",") + 1;
              while (strlen(first) && isblank(first[0]))
                first++;
              strstr(sp, ",")[0] = 0;
              
              middle = strrchr(first, ' ') + 1;
              while (strlen(middle) && isblank(middle[0]))
                middle++;
              strrchr(first, ' ')[0] = 0;
            }
          else
            {
              if (strlen(g_strstrip(sp)))
                {
                  first = sp;
                  last = strrchr(sp, ' ') + 1;
                  while (strlen(last) && isblank(last[0]))
                    last++;
                  strrchr(sp, ' ')[0] = 0;
                  middle = strrchr(first, ' ') + 1;
                  while (strlen(middle) && isblank(middle[0]))
                    middle++;
                  strrchr(first, ' ')[0] = 0;
                }
            }
        break;
      }
    db_set_data(p, "GIVEN_NAME", g_strdup(first));
    db_set_data(p, "MIDDLE_NAME", g_strdup(middle));
    db_set_data(p, "FAMILY_NAME", g_strdup(last));
    db_set_data(p, "TITLE", g_strdup(title));
    db_set_data(p, "HONORIFIC_SUFFIX", g_strdup(suffix));
}


void
on_edit_save_clicked (GtkButton * button, gpointer user_data)
{
  GtkWidget *edit = (GtkWidget *) user_data;
  GSList *tags;
  struct person *p = g_object_get_data (G_OBJECT (edit), "person");
  gchar *nametext = NULL;
  gboolean isdialog = (gboolean) g_object_get_data(G_OBJECT(edit), "isdialog");
  
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
              if (c->ignore_year)
                snprintf (buf, sizeof (buf) - 1, "0000%02d%02d", 
                  c->month, c->day);
              else
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
            text = g_strdup(g_object_get_data(G_OBJECT(w), "filename"));
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
      
      /* remember name */
      if (!strcasecmp(tag, "NAME"))
        {
            nametext = text;
        }
    }

    /* handle name details */
    if (nametext)
      store_name_fields(p, nametext);
  
  if (commit_person (p))
    {
      gtk_widget_destroy (edit);
      if (isdialog)
        {
          gtk_main_quit();
        }
      else
        {
          discard_person (p);
          update_display ();
        }
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
  const gchar *selected_filename =
    gtk_file_selection_get_filename (GTK_FILE_SELECTION (selector));
  
  if (selected_filename)
    {
       g_object_set_data(G_OBJECT(image), "filename", 
                         g_strdup(selected_filename));
       gtk_image_set_from_file(GTK_IMAGE(image), selected_filename);
    }
}

void
on_edit_bt_image_clicked (GtkWidget *bimage, gpointer user_data)
{
  GtkWidget *filesel = gtk_file_selection_new (_("Select image"));
  
  gtk_window_set_transient_for(GTK_WINDOW(filesel),
                               GTK_WINDOW(gtk_widget_get_toplevel(bimage)));
  gtk_window_set_modal(GTK_WINDOW(filesel), TRUE);
  gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(filesel));
  
  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		      "clicked", G_CALLBACK (store_filename), filesel);

  g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		    "clicked", G_CALLBACK (gtk_widget_destroy),
		    (gpointer) filesel);

  g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
		    "clicked", G_CALLBACK (gtk_widget_destroy),
		    (gpointer) filesel);
  
  g_object_set_data(G_OBJECT(filesel),"image", user_data);
  
  gtk_widget_show_all (filesel);
}

void
on_edit_cancel_clicked (GtkButton * button, gpointer user_data)
{
  GtkWidget *edit = user_data;
  if (g_object_get_data(G_OBJECT(edit), "isdialog"))
    {
      gtk_main_quit();
      exit(EXIT_SUCCESS);
    }
  else
    {
      update_display ();
    }
  gtk_widget_destroy (GTK_WIDGET (edit));
}

void
on_edit_window_closed_clicked (gpointer user_data)
{
  GtkWidget *edit = user_data;
  if (g_object_get_data(G_OBJECT(edit), "isdialog"))
    {
      gtk_main_quit();
      exit(EXIT_SUCCESS);
    }
  update_display ();
  gtk_widget_destroy (GTK_WIDGET (edit));
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
  g_object_set_data(G_OBJECT(gtk_widget_get_toplevel(widget)),
                    "inmultiline",(void*)1);  
  return FALSE;
}

gboolean 
tv_focus_out (GtkWidget *widget,
             GdkEventFocus *event,
             gpointer user_data)
{
  g_object_set_data(G_OBJECT(gtk_widget_get_toplevel(widget)),
                    "inmultiline",(void*)0);  
  return FALSE;
}

void
on_unknown_year_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
  GtkDateCombo *cb = user_data;
  
  gtk_date_combo_ignore_year(cb, gtk_toggle_button_get_active(togglebutton));
}

void
on_name_clicked (GtkButton *button, gpointer user_data)
{
  GtkWindow *edit = user_data;
  GtkEntry *e = g_object_get_data(G_OBJECT(button), "edit");
  struct person *p;
  gchar *name;

  p = g_object_get_data (G_OBJECT (edit), "person");
  
  name = gtk_editable_get_chars(GTK_EDITABLE(e), 0, -1);
  store_name_fields(p, name);
  g_free(name);
  if (do_edit_name_detail(edit, p))
    update_edit(p, GTK_WIDGET(edit));
}
