/*
 * Copyright (C) 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2005, 2006 Florian Boor <florian@kernelconcepts.de>
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
#include <gpe/contacts-db.h>
#include "main.h"
#include "namedetail.h"

#ifdef IS_HILDON
#include "pimc-ui.h"
#if HILDON_VER > 0
#include <hildon/hildon-file-chooser-dialog.h>
#include <hildon/hildon-caption.h>
#include <hildon/hildon-date-editor.h>
#else
#include <hildon-fm/hildon-widgets/hildon-file-chooser-dialog.h>
#include <hildon-widgets/hildon-caption.h>
#include <hildon-widgets/hildon-date-editor.h>
#include <hildon-widgets/hildon-input-mode-hint.h>
#endif /* HILDON_VER */
#endif

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
#ifdef IS_HILDON
static void on_date_set_toggled (GtkToggleButton *togglebutton, gpointer user_data);
#else
static void on_unknown_year_toggled (GtkToggleButton *togglebutton, gpointer user_data);
#endif
static void on_name_clicked (GtkButton *button, gpointer user_data);


/* CALLBACK : this is the filter for phone number edits */
void
on_phone_insert_text(GtkEditable *editable, gchar *new_text,
                     gint new_text_length, gint *position,
                     gpointer user_data)
{
  int i;
  gboolean isok = TRUE;
  
  for (i = 0; i < new_text_length; i++)
    if (!strchr(" +0123456789-/()", new_text[i]))
      {
        isok = FALSE;
        break;
      }
  if (!isok)
    gtk_signal_emit_stop_by_name(GTK_OBJECT(editable), "insert-text");
}

static void
add_tag (gchar *tag, GtkWidget *w, GtkWidget *pw)
{
  GSList *tags;
  g_object_set_data (G_OBJECT (w), "db-tag", tag);

  tags = g_object_steal_data (G_OBJECT (pw), "tag-widgets");
  tags = g_slist_append (tags, w);
  g_object_set_data_full (G_OBJECT (pw), "tag-widgets", tags, (GDestroyNotify) g_slist_free);
}

#ifdef IS_HILDON
static void
pop_singles (GtkWidget *vtable, GtkSizeGroup *size_group, GSList *list, GtkWidget *pw, gboolean visible, gint *pos)
#else
static void
pop_singles (GtkWidget *vtable, GSList *list, GtkWidget *pw, gboolean visible, gint *pos)
#endif
{
  if (list)
    {
      guint x = 0;
      gboolean alt_pos = FALSE; /* indicates left/right column on larger screens */
    
      while (list)
        {
          GSList *next = list->next;
          edit_thing_t e = list->data;
          GtkWidget *w = gtk_entry_new ();
          GtkWidget *l = NULL, *b = NULL;
          
          add_tag (e->tag, w, pw);
          if (strstr(e->tag,".TELEPHONE")
              || strstr(e->tag,".MOBILE") 
              || strstr(e->tag,".FAX"))
            {
#ifdef IS_HILDON
#if HILDON_VER > 0
	      /* Use Hildon telephone number mode */
	      hildon_gtk_entry_set_input_mode(GTK_ENTRY(w), (HILDON_GTK_INPUT_MODE_TELE));
#else
              g_signal_connect (G_OBJECT (w), "insert-text", 
		         G_CALLBACK (on_phone_insert_text), NULL);
#endif /* HILDON_VER */
              g_signal_connect (G_OBJECT (w), "insert-text", 
		         G_CALLBACK (on_phone_insert_text), NULL);
#endif /* IS_HILDON */
            }
#ifdef IS_HILDON
          if (strstr(e->tag,".WEB")
              || strstr(e->tag,".EMAIL")
              || strstr (e->tag, ".WWW"))
            {
#if HILDON_VER > 0
	      /* Turn off HILDON_GTK_INPUT_MODE_AUTOCAP */
	      hildon_gtk_entry_set_input_mode(GTK_ENTRY(w), (HILDON_GTK_INPUT_MODE_ALPHA | HILDON_GTK_INPUT_MODE_NUMERIC));
#else
  g_object_set (G_OBJECT (w), HILDON_AUTOCAP, FALSE, NULL);
#endif /* HILDON_VER */
              
            }

          l = hildon_caption_new (size_group, e->name, w, NULL, HILDON_CAPTION_OPTIONAL);
#else
          l = gtk_label_new (e->name);
          gtk_misc_set_alignment(GTK_MISC(l), 1.0, 0.5);
          if (!strcasecmp(e->tag, "NAME")) /* the name field on a button */
            {
              GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
              b = gtk_button_new_with_label(_("Details"));
              g_object_set_data(G_OBJECT(b), "edit", w);
              g_signal_connect(G_OBJECT(b), "clicked",
                               G_CALLBACK(on_name_clicked),
                               gtk_widget_get_toplevel(pw));
            }
#endif            
          /* if enough space: even fields right, odd fields left */
#ifdef IS_HILDON
          gtk_box_pack_start (GTK_BOX (vtable), l, FALSE, FALSE, 0);
#else
          if (alt_pos && mode_large_screen)
            {
              if (l) 
                gtk_table_attach(GTK_TABLE(vtable), l, 2, 3, *pos, *pos+1, 
                                 GTK_FILL, 0, 0, 0);
              gtk_table_attach(GTK_TABLE(vtable), w, 3, 4, *pos, *pos+1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
              if (b) 
                gtk_table_attach(GTK_TABLE(vtable), b, 4, 5, *pos, *pos+1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
            }
          else
            {
              (*pos)++;
              if (l) 
                gtk_table_attach(GTK_TABLE(vtable), l, 0, 1, *pos, *pos+1, 
                                 GTK_FILL, 0, 0, 0);
              if (mode_large_screen)
                {
                  if (!next && !b)
                    gtk_table_attach(GTK_TABLE(vtable), w, 1, 4, *pos, *pos+1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
                  else
                    gtk_table_attach(GTK_TABLE(vtable), w, 1, 2, *pos, *pos+1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
                  if (b)
                    gtk_table_attach(GTK_TABLE(vtable), b, 3, 4, *pos, *pos+1, 
                                     GTK_FILL, 0, 0, 0);
                }
              else
                {
                  if (!next && !b)
                    gtk_table_attach(GTK_TABLE(vtable), w, 1, 3, *pos, *pos+1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
                  else
                    gtk_table_attach(GTK_TABLE(vtable), w, 1, 2, *pos, *pos+1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
                  if (b)
                    gtk_table_attach(GTK_TABLE(vtable), b, 2, 3, *pos, *pos+1, 
                                    GTK_FILL, 0, 0, 0);

                }
            }
#endif
          g_slist_free_1 (list);
          list = next;
          x++;
 
#ifdef IS_HILDON
          if (visible)
            {
              gtk_widget_show_all (l);
            }
          else
            {
              gtk_widget_hide (l);
            }
#else
          if (visible)
            {
              if (l) gtk_widget_show(l);
              gtk_widget_show(w);
              if (b) gtk_widget_show(b);
            }
          else
            {
              if (l) gtk_widget_hide(l);
              gtk_widget_hide(w);
              if (b) gtk_widget_hide(b);
            }
#endif
        alt_pos = !alt_pos;
        }
    }
}

static void
build_children (GtkWidget *table, GSList *children, GtkWidget *pw, gboolean visible, gint* pos)
{
  GSList *child;
  GSList *singles = NULL;
#ifdef IS_HILDON
  GtkSizeGroup *size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
#endif

  for (child = children; child; child = child->next)
    {
      edit_thing_t e = child->data;
      GtkWidget *w = NULL, *ww;
      
      switch (e->type)
      {
        case GROUP:
          {
            gchar *markup = NULL;
#ifdef IS_HILDON
            pop_singles (table, size_group, singles, pw, visible, pos);            
#else
            pop_singles (table, singles, pw, visible, pos);
#endif
            if (e->name)
              {
                markup = g_strdup_printf ("<b>%s</b>", e->name);
                w = gtk_label_new (NULL);
                if (visible)
                  gtk_widget_show(w);
                else
                  gtk_widget_hide(w);
                gtk_label_set_markup (GTK_LABEL (w), markup);
                gtk_misc_set_alignment (GTK_MISC (w), 0, 0.5);
                g_free (markup);
                singles = NULL;
#ifdef IS_HILDON
                gtk_box_pack_start (GTK_BOX (table), w, FALSE, FALSE, 0);
#else
                (*pos)++;
                gtk_table_attach(GTK_TABLE(table), w, 0, 3, *pos, *pos+1, 
                                 GTK_FILL, 0, 0, 0);
#endif
                if (e->hidden)
                    gtk_widget_hide(w);
              }
            /* hidden group */
            build_children (table, e->children, pw, !e->hidden, pos);
          }
        break;
      
        case ITEM_MULTI_LINE:
#ifdef IS_HILDON
          pop_singles (table, size_group, singles, pw, visible, pos);            
#else
          pop_singles (table, singles, pw, visible, pos);
#endif
          singles = NULL;
          ww = gtk_text_view_new ();
#ifdef IS_HILDON
          w = hildon_caption_new (size_group, e->name, ww, NULL, HILDON_CAPTION_OPTIONAL);
          if (visible)
            gtk_widget_show_all (w);
          else
            gtk_widget_hide (w);
#else
          if (visible)
            gtk_widget_show(ww);
          else
            gtk_widget_hide(ww);
#endif
          gtk_text_view_set_editable (GTK_TEXT_VIEW (ww), TRUE);
          gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (ww), GTK_WRAP_WORD_CHAR);
          g_signal_connect(G_OBJECT(ww),"move_cursor",
            G_CALLBACK(tv_move_cursor),NULL);
          g_signal_connect(G_OBJECT(ww),"focus-in-event",
            G_CALLBACK(tv_focus_in),NULL);
          g_signal_connect(G_OBJECT(ww),"focus-out-event",
            G_CALLBACK(tv_focus_out),NULL);
          /*HACK: This is a workaround for a bug in the GtkTextView implementation 
            causing it to grow while entering text. */
          gtk_widget_set_size_request(ww, 5, -1);
          
#ifndef IS_HILDON
          gtk_widget_set_usize (GTK_WIDGET (ww), -1, 36);
          if (e->name)
            {
              w = gtk_label_new (e->name);
              gtk_misc_set_alignment(GTK_MISC(w), 1.0, 0.0);
              if (visible)
                gtk_widget_show(w);
              else
                gtk_widget_hide(w);
            }
#endif
          (*pos)++;
#ifdef IS_HILDON
          gtk_box_pack_start (GTK_BOX (table), w, FALSE, FALSE, 0);
#else
          if (e->name) gtk_table_attach(GTK_TABLE(table), w, 0, 1, *pos, *pos+1, 
                                        GTK_FILL, GTK_FILL, 0, 0);
          gtk_table_attach(GTK_TABLE(table), ww, 1, 3, *pos, *pos+1, 
                           GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
#endif
          add_tag (e->tag, ww, pw);
        break;
      
        case ITEM_SINGLE_LINE:
          singles = g_slist_append (singles, e);
        break;
      
        case ITEM_DATE:
        {
#ifdef IS_HILDON
          GtkWidget *date_set = gtk_check_button_new ();
          GtkWidget *date_editor = hildon_date_editor_new ();
          g_object_set(G_OBJECT(date_editor), "min-year", 1900, NULL );
          GtkWidget *hbox = gtk_hbox_new (FALSE, gpe_get_boxspacing ());
          GtkWidget *caption = hildon_caption_new (size_group, e->name, hbox, NULL, HILDON_CAPTION_OPTIONAL);

          gtk_box_pack_start (GTK_BOX (hbox), date_set, FALSE, FALSE, 0);
          gtk_box_pack_start (GTK_BOX (hbox), date_editor, FALSE, FALSE, 0);

          gtk_widget_set_sensitive (date_editor, FALSE);
          g_signal_connect (G_OBJECT (date_set), "toggled", G_CALLBACK (on_date_set_toggled), date_editor);
          g_object_set_data (G_OBJECT (date_editor), "date-set", date_set);

          pop_singles (table, size_group, singles, pw, visible, pos);
          singles = NULL;

          if (visible)
            {
              gtk_widget_show_all (caption);
            }
          else
            {
              gtk_widget_hide (caption);
            }

          gtk_box_pack_start (GTK_BOX (table), caption, FALSE, FALSE, 0);

          add_tag (e->tag, date_editor, pw);
#else
          GtkWidget *l = gtk_label_new (e->name);
          GtkWidget *datecombo, *cbnoyear;
          pop_singles (table, singles, pw, visible, pos);
          singles = NULL;
          
          gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);

          datecombo = gtk_date_combo_new ();
          gtk_date_combo_clear(GTK_DATE_COMBO(datecombo));
          cbnoyear = gtk_check_button_new_with_label (_("no year"));
          g_signal_connect (G_OBJECT (cbnoyear), "toggled",
		    G_CALLBACK (on_unknown_year_toggled), datecombo);
          g_object_set_data(G_OBJECT(datecombo),"cbnoyear",cbnoyear);
          if (visible)
            {
              gtk_widget_show(l);
              gtk_widget_show(datecombo);
              gtk_widget_show(cbnoyear);
            }
          else
            {
              gtk_widget_hide(l);
              gtk_widget_hide(datecombo);
              gtk_widget_hide(cbnoyear);
            }
          (*pos)++;
          gtk_table_attach(GTK_TABLE(table), l, 0, 1, *pos, *pos+1, 
                           GTK_FILL, 0, 0, 0);
          gtk_table_attach(GTK_TABLE(table), datecombo, 1, 2, *pos, *pos+1, 0, 0, 0, 0);
          gtk_table_attach(GTK_TABLE(table), cbnoyear, 2, 3, *pos, *pos+1, 0, 0, 0, 0);
          add_tag (e->tag, datecombo, pw);
#endif
        }
        break;
        case ITEM_IMAGE:
        {
          GtkWidget *l = NULL;
          GtkWidget *image, *btn, *sw, *vp;
#ifdef IS_HILDON
          pop_singles (table, size_group, singles, pw, visible, pos);            
          singles = NULL;

#else
          pop_singles (table, singles, pw, visible, pos);
          singles = NULL;
          
          if (e->name)
            {
               l = gtk_label_new (e->name);
               gtk_misc_set_alignment (GTK_MISC (l), 0.0, 0.5);
               gtk_widget_show(l);
            }
#endif
          btn = gtk_button_new();
          image = gtk_image_new();
          vp = gtk_viewport_new(NULL, NULL);
          gtk_viewport_set_shadow_type(GTK_VIEWPORT(vp), GTK_SHADOW_NONE);
          sw = gtk_scrolled_window_new(NULL, NULL);
          gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), 
                                         GTK_POLICY_AUTOMATIC,
                                         GTK_POLICY_AUTOMATIC);
          gtk_container_add(GTK_CONTAINER(vp), btn);
          gtk_container_add(GTK_CONTAINER(sw), vp);
          gtk_container_add(GTK_CONTAINER(btn), image);
          gtk_widget_show_all(sw);
          g_signal_connect (G_OBJECT (btn), "clicked",
		                    G_CALLBACK (on_edit_bt_image_clicked), image);
#ifdef IS_HILDON
          l = hildon_caption_new (size_group, e->name ? e->name : _("Image"), sw, NULL, HILDON_CAPTION_OPTIONAL);
          gtk_widget_show (l);
          gtk_widget_set_size_request(image, IMG_WIDTH, IMG_HEIGHT);
          gtk_widget_set_size_request(vp, IMG_WIDTH+10, IMG_HEIGHT+10);
          gtk_box_pack_start (GTK_BOX (table), l, FALSE, FALSE, 0);
#else
          (*pos)++;
          if (e->name)
              gtk_table_attach(GTK_TABLE(table), l, 0, 1, *pos, *pos+1, 
                               GTK_FILL, 0, 0, 0);
          gtk_table_attach_defaults(GTK_TABLE(table), sw, 1, 3, *pos, *pos+1);
#endif          
          add_tag (e->tag, image, pw);
        }
        default:
          /* just ignore */
        break;
      }
    }

#ifdef IS_HILDON
  pop_singles (table, size_group, singles, pw, visible, pos);            
#else
  pop_singles (table, singles, pw, visible, pos);
#endif
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
  GtkWidget *vbox, *action_area;
    
  if (mode_large_screen)
    {
      edit = gtk_dialog_new ();
#ifdef IS_HILDON
      gtk_window_set_transient_for(GTK_WINDOW(edit), 
                                   GTK_WINDOW(gtk_widget_get_toplevel(mainw)));
#else
      gtk_window_set_transient_for(GTK_WINDOW(edit), GTK_WINDOW(mainw));
#endif
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
 
#ifdef IS_HILDON
  edit_cancel = gtk_button_new_with_label (_("Cancel"));
  edit_save = gtk_button_new_with_label (_("OK"));
#else
  if (mode_landscape || mode_large_screen)
    edit_delete = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  edit_cancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  edit_save = gtk_button_new_from_stock (GTK_STOCK_SAVE);
#endif
  
  GTK_WIDGET_SET_FLAGS (edit_save, GTK_CAN_DEFAULT);

#ifdef IS_HILDON
  gtk_box_pack_start (GTK_BOX (action_area), edit_save, TRUE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX (action_area), edit_cancel, TRUE, FALSE, 4);
#else
  if (mode_landscape || mode_large_screen)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (edit_delete), FALSE);
      gtk_box_pack_start (GTK_BOX (action_area), edit_delete, TRUE, FALSE, 4);
    }
  gtk_box_pack_start (GTK_BOX (action_area), edit_cancel, TRUE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX (action_area), edit_save, TRUE, FALSE, 4);
#endif
    
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

  g_object_set_data (G_OBJECT (edit), "notebook2", notebook2);

  if (mode_large_screen)
    {
#ifdef IS_HILDON    
        gtk_window_set_default_size (GTK_WINDOW (edit), 620, 400);
#else
        gtk_window_set_default_size (GTK_WINDOW (edit), 320, 480);
#endif
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
  /* Create the window */
  GtkWidget *w = create_edit ();
  GtkWidget *book = lookup_widget (w, "notebook2");
  GSList *page;

  gtk_widget_show_all(w);

  for (page = edit_pages; page; page = page->next)
    {
      edit_thing_t e = page->data;
      gint pos = 0;
#ifdef IS_HILDON
      GtkWidget *table = gtk_vbox_new (FALSE, gpe_get_boxspacing());
#else
      GtkWidget *table = gtk_table_new (3, 2, FALSE);
#endif
      GtkWidget *label = gtk_label_new (e->name);
      GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
      GtkWidget *viewport = gtk_viewport_new(NULL, NULL);

      gtk_container_set_border_width (GTK_CONTAINER (table), gpe_get_border());
#ifndef IS_HILDON
      gtk_table_set_row_spacings(GTK_TABLE(table), gpe_get_boxspacing());
      gtk_table_set_col_spacings(GTK_TABLE(table), gpe_get_boxspacing());
#endif

      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
      gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport), GTK_SHADOW_NONE);
      gtk_container_add(GTK_CONTAINER(viewport), table);
      gtk_container_add(GTK_CONTAINER(scrolled_window), viewport);
      gtk_widget_show_all(scrolled_window);
      gtk_widget_show(label);
      gtk_notebook_append_page (GTK_NOTEBOOK (book), scrolled_window, label);
      build_children (table, e->children, w, TRUE, &pos);
      /* add categories stuff to the last page */
      if (!page->next)
        {
#ifdef IS_HILDON
          GtkWidget *list = build_pim_categories_list();
          
          gtk_box_pack_start (GTK_BOX (table), list, TRUE, TRUE, 0);
          g_object_set_data(G_OBJECT(w), "categories", list);
#else            
          GtkWidget *cathbox, *catbutton, *catlabel;
          cathbox = gtk_hbox_new (FALSE, gpe_get_boxspacing());
          catbutton = gtk_button_new_with_label (_("Categories"));
          catlabel = gtk_label_new (NULL);
          gtk_misc_set_alignment (GTK_MISC (catlabel), 0.0, 0.5);  
          gtk_box_pack_start (GTK_BOX (cathbox), catbutton, FALSE, FALSE, gpe_get_boxspacing());
          gtk_box_pack_start (GTK_BOX (cathbox), catlabel, TRUE, TRUE, 0);
          pos++;
          gtk_table_attach (GTK_TABLE (table), cathbox, 0, 3, pos, pos+1, 
                            GTK_FILL, 0, 0, 0);
          g_signal_connect (G_OBJECT (catbutton), "clicked",
                            G_CALLBACK (on_categories_clicked), w);
          g_object_set_data (G_OBJECT (w), "categories-label", catlabel);
          if (!isdialog)
            gtk_widget_show_all(cathbox);
#endif
	  
        }
    }

  return w;
}

GSList *
get_categories_list (struct contacts_person *p)
{
  GSList *iter;
  GSList *list = NULL;

  iter = p->data;

  for (; iter; iter = iter->next)
    {
      struct contacts_tag_value *t = iter->data;
      if (!strcasecmp (t->tag, "category"))
        {
          int id = atoi (t->value);
          list = g_slist_prepend (list, (gpointer)id);
	    }
    }

  return list;
}

gchar *
build_categories_string (struct contacts_person *p)
{
  gchar *s = NULL;
  GSList *iter, *list;

  list = get_categories_list (p);

  for (iter = list; iter; iter = iter->next)
    {
      const gchar *cat;
      const gchar *col;
      cat = gpe_pim_category_name ((gint)iter->data);
      col = gpe_pim_category_colour ((gint)iter->data);
        
      if (cat)
        {
          if (s)
            {
              gchar *ns = 
                col ? g_strdup_printf ("%s, <span underline=\"single\" "\
                                       "underline_color=\"%s\">%s</span>", s, col, cat) 
                    : g_strdup_printf ("%s, %s", s, cat);
              g_free (s);
              s = ns;
            }
          else
              s = col ? g_strdup_printf ("<span underline=\"single\" "\
                                         "underline_color=\"%s\">%s</span>", col, cat) 
                    : g_strdup (cat);
        }
    }

  g_slist_free (list);

  return s;
}

static void
update_categories_list (GtkWidget *ui, GSList *selected, GtkWidget *edit)
{
  struct contacts_person *p;
#ifndef IS_HILDON
  GtkWidget *w;
  gchar *str;
#endif  
  GSList *iter;

  p = g_object_get_data (G_OBJECT (edit), "person");
  contacts_db_delete_tag (p, "CATEGORY");

  for (iter = selected; iter; iter = iter->next)
    {
      contacts_db_set_multi_data (p, "CATEGORY", g_strdup_printf ("%d", (int)iter->data));
    }

#ifndef IS_HILDON
  str = build_categories_string (p);
  w = lookup_widget (edit, "categories-label");
  gtk_label_set_markup (GTK_LABEL (w), str);
  g_free (str);
#endif
}

void
edit_person (struct contacts_person *p, gchar *title, gboolean isdialog)
{
  /* Create the window */
  GtkWidget *window = edit_window (isdialog);

#ifdef IS_HILDON    
  GSList *catlist = NULL;
#else    
  gchar *str;
  GtkWidget *catlabel;
#endif
    
  gtk_window_set_title (GTK_WINDOW (window), title);
  g_object_set_data(G_OBJECT(window), "isdialog", (gpointer)isdialog);  
  
  if (p)
    {
      GSList *tags = gtk_object_get_data (GTK_OBJECT (window), "tag-widgets");

      /* For all widgets corresponding to editable tags */
      GSList *iter;
      for (iter = tags; iter; iter = iter->next)
        {
          GtkWidget *widget = iter->data;

	  /* Get the data associated to the corresponding tag in the database */
          gchar *tag = gtk_object_get_data (GTK_OBJECT (widget), "db-tag");
          struct contacts_tag_value *v = contacts_db_find_tag (p, tag);

          guint pos = 0;
          if (tag)
            {
               if (!strcasecmp(tag, "NAME"))
                 {
                   gtk_widget_grab_default(widget);
                   gtk_widget_grab_focus(widget);
                 }
            }
          if (v && v->value)
            {
              if (GTK_IS_EDITABLE (widget))
                {
                  gtk_editable_insert_text (GTK_EDITABLE (widget), v->value,
                          strlen (v->value), &pos);
                }
#ifdef IS_HILDON
              else if (HILDON_IS_DATE_EDITOR (widget))
                {
                  GtkToggleButton *date_set = g_object_get_data (G_OBJECT (widget), "date-set");
                  gtk_toggle_button_set_active (date_set, v->value != NULL);

                  if (v->value)
                    {
                      guint year, month, day;
                      
                      sscanf (v->value, "%04d%02d%02d", &year, &month, &day);
                      hildon_date_editor_set_date (HILDON_DATE_EDITOR (widget), year, month, day);
                    }
                }
#else
              else if (GTK_IS_DATE_COMBO(widget))
                {
                  if (v->value)
                    {
                      guint year, month, day;
                      GtkToggleButton *cbnoyear;
                      
                      sscanf (v->value, "%04d%02d%02d", &year, &month, &day);
                      gtk_date_combo_set_date (GTK_DATE_COMBO (widget), year, month - 1, day);
                      if (year == 0)
                        {
                          gtk_date_combo_ignore_year(GTK_DATE_COMBO(widget),TRUE);
                          cbnoyear = g_object_get_data(G_OBJECT(widget),"cbnoyear");
                          gtk_toggle_button_set_active(cbnoyear,TRUE);
                        }
                    }
                  else
                    gtk_date_combo_clear (GTK_DATE_COMBO (widget));
                }
#endif
              else if (GTK_IS_IMAGE(widget))
                {
                  if ((v->value) && !access(v->value,R_OK)) 
                    {
                      GdkPixbuf *buf;
                      g_object_set_data(G_OBJECT(widget), "filename", v->value);
                      buf = gdk_pixbuf_new_from_file_at_size(v->value, IMG_WIDTH, 
                                                             IMG_HEIGHT, NULL);
                      gtk_image_set_from_pixbuf(GTK_IMAGE(widget), buf);
                      gdk_pixbuf_unref(buf);
                    }
                  else
                    gtk_image_set_from_stock(GTK_IMAGE(widget), 
                      GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_BUTTON);
                }
              else  
                gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget)), 
                          v->value, -1);
            }
        }
      gtk_object_set_data (GTK_OBJECT (window), "person", p);

#ifdef IS_HILDON
      catlist = get_categories_list (p);
#else
      /* display categories */  
      str = build_categories_string (p);
      catlabel = lookup_widget (window, "categories-label");
      gtk_label_set_markup (GTK_LABEL (catlabel), str);
      g_free (str);
#endif        
    }
#ifdef IS_HILDON
  populate_pim_categories_list(g_object_get_data(G_OBJECT(window), "categories"), 
                               catlist);
  if (catlist)
      g_slist_free(catlist);
#endif  
  gtk_widget_show (window);
}

/* Concatenate name parts. 
   caller is responsible for freeing returned value */

gchar *concatenate_name_parts(gchar *n1, gchar *n2, gchar *n3, gchar *n4,gchar *n5)
{
  gchar *ts = g_strdup("");
  if (n1 && strlen(n1))
    {
      n1 = g_strdup_printf("%s%s ", ts, g_strstrip(n1));
      g_free(ts);
      ts = n1;
    }
  if (n2 && strlen(n2))
    {
      n1 = g_strdup_printf("%s%s ", ts, g_strstrip(n2));
      g_free(ts);
      ts = n1;
    }
  if (n3 && strlen(n3)) 
    {
      n1 = g_strdup_printf("%s%s ", ts, g_strstrip(n3));
      g_free(ts);
      ts = n1;
    }
  if (n4 && strlen(n4)) 
    {
      n1 = g_strdup_printf("%s%s ", ts, g_strstrip(n4));
      g_free(ts);
      ts = n1;
    }
  if (n5 && strlen(n5)) 
    {
      n1 = g_strdup_printf("%s%s", ts, g_strstrip(n5));
      g_free(ts);
      ts = n1;
    }
  g_strstrip(ts);
  return ts;
}

void
update_edit (struct contacts_person *p, GtkWidget *window)
{
  GtkWidget *nameentry[6];
  int namenum = 0;
  int i;
  gchar *namepart[6];

  for(i = 0; i < 6; i++)
    {
      nameentry[i] = NULL;
      namepart[i] = NULL;
    }

  if (p)
    {
      GSList *tags = gtk_object_get_data (GTK_OBJECT (window), "tag-widgets");

      /* For every widgets corresponding to tags in the window */
      GSList *iter;
      for (iter = tags; iter; iter = iter->next)
        {
          GtkWidget *widget = iter->data;
          gchar *tag = gtk_object_get_data (GTK_OBJECT (widget), "db-tag");
          struct contacts_tag_value *v = contacts_db_find_tag (p, tag);
          guint pos = 0;
          if (v && v->value)
            {
	      /* collect data for name field update */
	      if (!strcasecmp(tag,"NAME"))
		{
		  nameentry[5] = widget;
		  namepart[5] = v->value;
		}
              if (!strcasecmp(tag,"TITLE"))
		{
		  nameentry[0] = widget;
		  namepart[0] = v->value;
		  namenum++;
		}
              if (!strcasecmp(tag,"GIVEN_NAME"))
		{
		  nameentry[1] = widget;
		  namepart[1] = v->value;
		  namenum++;
		}
              if (!strcasecmp(tag,"MIDDLE_NAME"))
		{
		  nameentry[2] = widget;
		  namepart[2] = v->value;
		  namenum++;
		}
              if (!strcasecmp(tag,"FAMILY_NAME"))
		{
		  nameentry[3] = widget;
		  namepart[3] = v->value;
		  namenum++;
		}
              if (!strcasecmp(tag,"HONORIFIC_SUFFIX"))
		{
		  nameentry[4] = widget;
		  namepart[4] = v->value;
		  namenum++;
		}

              if (GTK_IS_EDITABLE (widget))
                {
                  gtk_editable_delete_text(GTK_EDITABLE(widget), 0, -1);
                  gtk_editable_insert_text (GTK_EDITABLE (widget), v->value,
                                            strlen (v->value), &pos);
                }
#ifdef IS_HILDON
              else if (HILDON_IS_DATE_EDITOR (widget))
                {
                  GtkToggleButton *date_set = g_object_get_data (G_OBJECT (widget), "date-set");
                  gtk_toggle_button_set_active (date_set, v->value != NULL);

                  if (v->value)
                    {
                      guint year, month, day;
                      
                      sscanf (v->value, "%04d%02d%02d", &year, &month, &day);
                      hildon_date_editor_set_date (HILDON_DATE_EDITOR (widget), year, month, day);
                    }
                }
#else
              else if (GTK_IS_DATE_COMBO(widget))
                {
                  if (v->value)
                    {
                      guint year, month, day;
                      GtkToggleButton *cbnoyear;
                      
                      sscanf (v->value, "%04d%02d%02d", &year, &month, &day);
                      gtk_date_combo_set_date (GTK_DATE_COMBO (widget), year, month - 1, day);
                      if (year == 0)
                        {
                          gtk_date_combo_ignore_year(GTK_DATE_COMBO(widget),TRUE);
                          cbnoyear = g_object_get_data(G_OBJECT(widget),"cbnoyear");
                          gtk_toggle_button_set_active(cbnoyear,TRUE);
                        }
                    }
                  else
                    gtk_date_combo_clear (GTK_DATE_COMBO (widget));
                }
#endif
              else if (GTK_IS_IMAGE(widget))
                {
                  g_object_set_data(G_OBJECT(widget),"filename",v->value);
                  if ((v->value) && !access(v->value,R_OK))
                    {
                      GdkPixbuf *buf;
                      g_object_set_data(G_OBJECT(widget), "filename", v->value);
                      buf = gdk_pixbuf_new_from_file_at_size(v->value, IMG_WIDTH, 
                                                             IMG_HEIGHT, NULL);
                      gtk_image_set_from_pixbuf(GTK_IMAGE(widget), buf);
                      gdk_pixbuf_unref(buf);
                    }
                  else
                    gtk_image_set_from_stock(GTK_IMAGE(widget), 
                      GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_BUTTON);
                }
             else if (GTK_IS_TEXT_VIEW(widget))
                gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget)), 
                          v->value, -1);
            }

        }
    }

  if (namenum)
    {
      gchar *ts = concatenate_name_parts(namepart[0], namepart[1], namepart[2], namepart[3], namepart[4]);

      /* First update value of concatenated name */
      contacts_db_set_data(p, "NAME", g_strdup(ts));
      if (nameentry[5])
	gtk_entry_set_text(GTK_ENTRY(nameentry[5]), ts);
      g_free(ts);

      /* Then the sub-components */
      for (i = 0; i < 5; i++)
	{
	  if (nameentry[i])
	    {
	      gtk_entry_set_text(GTK_ENTRY(nameentry[i]), namepart[i]);
	    }
	}

    }
}


void
store_name_fields(struct contacts_person *p, const char *nametext)
{
  char e[4][100];
  int num_elements, i;
  gboolean has_title = FALSE, has_suffix = FALSE;
  char *sp;
  /*  char *origsp; */
  char *title = NULL;
  char *first = NULL;
  char *middle = NULL;
  char *last = NULL;
  char *suffix = NULL;
  char *refsuffix = NULL;
   
  if (!nametext) return;
  if (strlen(nametext) < 3) return;

  sp = g_strdup(nametext);
  /*  origsp = sp; */

  /* scan first element for title */
  if (sscanf(sp, "%s", e[0]))
      {
        struct contacts_tag_value *v;
        i = 0;
        
        while (titles[i])
          {
            if (!strcasecmp(e[0], gettext(titles[i])))
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
            v = contacts_db_find_tag(p, "TITLE");
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
	    refsuffix = gettext(suffixes[i]);
            if (!strcmp(suffix, refsuffix))
              {
                char *t;
                has_suffix = TRUE;
                t = strstr(sp, refsuffix);
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
              last = strrchr(sp, ' ');
              if (last != NULL)
                {
                  last++;
                  while (strlen(last) && isblank(last[0]))
                    last++;
                  strrchr(sp, ' ')[0] = 0;
                }
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
              
              middle = strrchr(first, ' ');
              if (middle != NULL)
                {
                  middle++;
                  while (strlen(middle) && isblank(middle[0]))
                    middle++;
                  strrchr(first, ' ')[0] = 0;
                }
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
                  middle = strrchr(first, ' ');
                  if (middle != NULL)
                    {
                      middle++;
                      while (strlen(middle) && isblank(middle[0]))
                        middle++;
                      strrchr(first, ' ')[0] = 0;
                    }
                }
            }
	  break;
      }
    contacts_db_set_data(p, "GIVEN_NAME", g_strdup(first));
    contacts_db_set_data(p, "MIDDLE_NAME", g_strdup(middle));
    contacts_db_set_data(p, "FAMILY_NAME", g_strdup(last));
    contacts_db_set_data(p, "TITLE", g_strdup(title));
    contacts_db_set_data(p, "HONORIFIC_SUFFIX", g_strdup(suffix));

    /*    g_free(origsp); */

}


/* CALLBACK */

void
on_edit_save_clicked (GtkButton * button, gpointer user_data)
{
  GtkWidget *edit = (GtkWidget *) user_data;
  GSList *tags;
  struct contacts_person *p = g_object_get_data (G_OBJECT (edit), "person");
  gchar *nametext = NULL;
  gboolean isdialog = (gboolean) g_object_get_data(G_OBJECT(edit), "isdialog");
  gchar *n1 = NULL, *n2 = NULL, *n3 = NULL, *n4 = NULL, *n5 = NULL;
  
  for (tags = g_object_get_data (G_OBJECT (edit), "tag-widgets");
       tags; tags = tags->next)
    {
      GtkWidget *w = tags->data;
      gchar *text, *tag;
      gchar buf[32];
      
      if (GTK_IS_EDITABLE (w))
        text = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
#ifdef IS_HILDON
      else if (HILDON_IS_DATE_EDITOR (w))
        {
          GtkToggleButton *date_set = g_object_get_data (G_OBJECT (w), "date-set");

          if (gtk_toggle_button_get_active (date_set))
            {
              guint year, month, day;
              hildon_date_editor_get_date (HILDON_DATE_EDITOR (w), &year, &month, &day);
              snprintf (buf, sizeof (buf) - 1, "%04d%02d%02d", year, month, day);
              text = g_strdup (buf);
            }
          else
            {
              text = NULL;
            }
        }
#else
      else if (GTK_IS_DATE_COMBO(w))
        {
          GtkDateCombo *c = GTK_DATE_COMBO (w);
          if (c->set)
            {
              if (c->ignore_year)
                snprintf (buf, sizeof (buf) - 1, "0000%02d%02d", 
                  c->month + 1, c->day);
              else
                snprintf (buf, sizeof (buf) - 1, "%04d%02d%02d", 
                  c->year, c->month + 1, c->day);
              buf[sizeof (buf) - 1] = 0;
              text = g_strdup(buf);
            }
          else
            text = NULL;
        }
#endif
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
      contacts_db_set_data (p, tag, text);
      
      /* remember name details components for later comparison */
      if (!strcasecmp(tag,"TITLE"))
	{
	  n1 = text;
	}
      if (!strcasecmp(tag,"GIVEN_NAME"))
	{
	  n2 = text;
	}
      if (!strcasecmp(tag,"MIDDLE_NAME"))
	{
	  n3 = text;
	}
      if (!strcasecmp(tag,"FAMILY_NAME"))
	{
	  n4 = text;
	}
      if (!strcasecmp(tag,"HONORIFIC_SUFFIX"))
	{
	  n5 = text;
	}

      if (!strcasecmp(tag, "NAME"))
        {
	  nametext = text;
        }
    }

    /* handle name modification */
    if (nametext)
      {
	/* concatenate name details parts to compare previous values
	   to potential recent change of "full" name */
	gchar *oldnametext = concatenate_name_parts(n1, n2, n3, n4, n5);
	
	/* only if "full" name was changed since previous edition of
	   name details, we need to parse again the name details and
	   update name details values accordingly */
	if(strcmp(nametext, oldnametext))
	  store_name_fields(p, nametext);

	g_free(oldnametext);
      }

#ifdef IS_HILDON
    {   
        GtkWidget *clist = g_object_get_data(G_OBJECT(edit), "categories");
        GSList *categories = get_categories(clist);
        /* hildon app need to take care about categories itself */
        update_categories_list(NULL, categories, edit);
        g_slist_free(categories);
    } 
#endif
  if (contacts_commit_person (p))
    {
      gtk_widget_destroy (edit);
      if (isdialog)
        {
          gtk_main_quit();
        }
      else
        {
          contacts_discard_person (p);
          update_display ();
        }
    }
}

void
on_categories_clicked (GtkButton *button, gpointer user_data)
{
  struct contacts_person *p;
  GtkWidget *w;

  p = g_object_get_data (G_OBJECT (user_data), "person");

#ifdef IS_HILDON
  w = gpe_pim_categories_dialog (get_categories_list (p), TRUE,
        G_CALLBACK (update_categories_list), user_data);
#else
  w = gpe_pim_categories_dialog (get_categories_list (p), 
        G_CALLBACK (update_categories_list), user_data);
#endif  
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
       GdkPixbuf *buf;
       g_object_set_data(G_OBJECT(image), "filename", 
                         g_strdup(selected_filename));
       buf = gdk_pixbuf_new_from_file_at_size(selected_filename, IMG_WIDTH, 
                                              IMG_HEIGHT, NULL);
       gtk_image_set_from_pixbuf(GTK_IMAGE(image), buf);
       gdk_pixbuf_unref(buf);
    }
}

void
on_edit_bt_image_clicked (GtkWidget *bimage, gpointer user_data)
{
#ifdef IS_HILDON
  GtkWidget *image = (GtkWidget*)user_data;
  GtkWidget *filesel = hildon_file_chooser_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(bimage)), 
                                                      GTK_FILE_CHOOSER_ACTION_OPEN);
  gtk_widget_show_all(filesel);
  if (gtk_dialog_run(GTK_DIALOG(filesel)) == GTK_RESPONSE_OK)
    {
      const gchar *selected_filename =
        gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filesel));
      if (selected_filename)
        {
           GdkPixbuf *buf;
           g_object_set_data(G_OBJECT(image), "filename", 
                             g_strdup(selected_filename));
           buf = gdk_pixbuf_new_from_file_at_size(selected_filename, IMG_WIDTH, 
                                                  IMG_HEIGHT, NULL);
           gtk_image_set_from_pixbuf(GTK_IMAGE(image), buf);
           gdk_pixbuf_unref(buf);
        }
    }
  gtk_widget_destroy(filesel);  
#else
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
#endif
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

#ifdef IS_HILDON
static void
on_date_set_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
  gtk_widget_set_sensitive (GTK_WIDGET (user_data), gtk_toggle_button_get_active (togglebutton));
}
#else
static void
on_unknown_year_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
  GtkDateCombo *cb = user_data;
  
  gtk_date_combo_ignore_year(cb, gtk_toggle_button_get_active(togglebutton));
}
#endif

/* CALLBACK */

static void
on_name_clicked (GtkButton *button, gpointer user_data)
{
  GtkWindow *edit = user_data;
  GtkEntry *e = g_object_get_data(G_OBJECT(button), "edit");
  struct contacts_person *p;
  gchar *oldname = "";
  gchar *name = NULL;
  struct contacts_tag_value *v;
 
  p = g_object_get_data (G_OBJECT (edit), "person");
  
  /* get the name displayed in the widget */
  name = gtk_editable_get_chars(GTK_EDITABLE(e), 0, -1);

  /* get the name in the person's tags */
  v = contacts_db_find_tag(p,"NAME");
  if (v && v->value) 
    oldname = v->value;

  /* if both names differ, name was edited */
  if(strcmp(oldname,name))
    {
    
      /* so split the name into parts before editing details */
      store_name_fields(p, name);
    }
  g_free(name);

  /* Display the name details dialog */
  if (do_edit_name_detail(edit, p))
    {
      /* Then update contents of name widget */
      update_edit(p, GTK_WIDGET(edit));
    }
}
