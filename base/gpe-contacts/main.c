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
#include <time.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/smallbox.h>
#include <gpe/errorbox.h>
#include <gpe/gtkdatecombo.h>
#include <gpe/question.h>
#include <gpe/gtksimplemenu.h>
#include <gpe/picturebutton.h>
#include <gpe/spacing.h>

#include "support.h"
#include "db.h"
#include "structure.h"
#include "proto.h"
#include "main.h"
#include "export.h"

#define SYSTEM_PIXMAPS_DIR PREFIX "/share/pixmaps/"
#define MY_PIXMAPS_DIR "gpe-contacts/"
#define TAB_CONFIG_LOCAL ".contacts-tab"
#define TAB_CONFIG_GLOBAL "/etc/gpe/contacts-tab.conf"

GtkWidget *categories_smenu;
GtkWidget *mainw;
GtkListStore *list_store;
GtkWidget *list_view;
GtkWidget *search_entry;
GtkWidget *popup_menu;
GtkWidget *bluetooth_menu_item;
guint menu_uid;
gboolean mode_landscape;
gboolean mode_large_screen;

struct gpe_icon my_icons[] = {
  {"edit"},
  {"delete"},
  {"cancel"},
  {"frame", MY_PIXMAPS_DIR "frame"},
  {"notebook", MY_PIXMAPS_DIR "notebook"},
  {"entry", MY_PIXMAPS_DIR "entry"},
  {"icon", SYSTEM_PIXMAPS_DIR "gpe-contacts.png" },
  {NULL, NULL}
};

static void
menu_do_edit (void)
{
  struct person *p;
  p = db_get_by_uid (menu_uid);
  edit_person (p);
}

static void
menu_do_delete (void)
{
  if (gpe_question_ask (_("Really delete this contact?"), _("Confirm"), 
			"question", "!gtk-cancel", NULL, "!gtk-delete", NULL, NULL) == 1)
    {
      if (db_delete_by_uid (menu_uid))
	update_display ();
    }
}

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
			    "question", "!gtk-cancel", NULL, "!gtk-delete", NULL, NULL) == 1)
	{
	  if (db_delete_by_uid (uid))
	    update_display ();
	}
    }
}

const gchar*
get_tag_name_1(const gchar *tag, edit_thing_t e)
{
  GSList *iter;
  const gchar* ret = NULL;

  switch (e->type)
    {
      case GROUP:
      case PAGE:
        for (iter = e->children; iter && !ret; iter = iter->next)
          ret = get_tag_name_1 (tag, iter->data);
      break;

      case ITEM_DATE:
      case ITEM_IMAGE:
      case ITEM_SINGLE_LINE:
      case ITEM_MULTI_LINE:
        if (!strcmp(tag,e->tag))
          return e->name;
      break;
    } 
    return ret;
}

gchar*
get_tag_name(const gchar* tag)
{
  GSList *page;

  for (page = edit_pages; page; page = page->next)
    {
      edit_thing_t e = page->data;

      return g_strdup(get_tag_name_1 (tag, e));
    }

  return NULL;
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
  GSList *iter;
  GList *wlist, *witer;
  gchar *fieldname, *fielddesc;

  table = GTK_TABLE (lookup_widget (mainw, "tabDetail"));
  
  
  if (mode_landscape) /* we show all available data */
    {
      wlist = gtk_container_get_children (GTK_CONTAINER (table));
      
      if (wlist != NULL)
        {
          for (witer = wlist; witer; witer = witer->next)
            gtk_container_remove (GTK_CONTAINER (table), GTK_WIDGET (witer->data));
          g_list_free (wlist);
        }
      i = 1;
      if (p != NULL)
        for (iter = p->data; iter; iter = iter->next)
          {
            struct tag_value *id = (struct tag_value *) iter->data;
            fieldname = get_tag_name(id->tag);
            if ((fieldname) && strcmp(id->tag,"NAME"))
              {
                if (fieldname[strlen(fieldname)-1] == ':')
                  fieldname[strlen(fieldname)-1] = 0;
                if (strstr(id->tag,"HOME"))
                  fielddesc = g_strdup_printf("%s (%s):",fieldname,_("home"));
                else if (strstr(id->tag,"WORK"))
                  fielddesc = g_strdup_printf("%s (%s):",fieldname,_("work"));
                else
                  fielddesc = g_strdup_printf("%s:",fieldname);
                lleft = gtk_label_new(fielddesc);
                gtk_misc_set_alignment(GTK_MISC(lleft),0,0);
                gtk_table_attach(table, lleft, 0, 1, i, i+1, GTK_FILL, GTK_FILL, 0, 0);
                lright = gtk_label_new(id->value);
                gtk_misc_set_alignment(GTK_MISC(lright),0,0);
                gtk_table_attach(table, lright, 1, 2, i, i+1, GTK_FILL, GTK_FILL, 0, 0);
                i++;
                g_free(fieldname);
                g_free(fielddesc);
              }
            else /* handle special fields */
              {
                gchar *ts;
                if (!strcmp(id->tag,"NAME")) 
                 {
                    if (fieldname[strlen(fieldname)-1] == ':')
                      fieldname[strlen(fieldname)-1] = 0;
                    lleft = gtk_label_new(NULL);
                    ts = g_strdup_printf("<b>%s:</b>",_("Name"));
                    gtk_label_set_markup(GTK_LABEL(lleft),ts);
                    g_free(ts);
                    gtk_misc_set_alignment(GTK_MISC(lleft),0,0);
                    gtk_table_attach(table, lleft, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
                    lright = gtk_label_new(NULL);
                    ts = g_strdup_printf("<b>%s</b>",id->value);
                    gtk_label_set_markup(GTK_LABEL(lright),ts);
                    g_free(ts);
                    gtk_misc_set_alignment(GTK_MISC(lright),0,0);
                    gtk_table_attach(table, lright, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
                 }
                if (!strcmp(id->tag,"BIRTHDAY") && strlen(id->value)>=8) 
                  {
                    int year,month,day;
                    struct tm tm;
                    char buf[32];

                    memset(&tm, 0, sizeof(tm));
                    sscanf(id->value,"%04d%02d%02d",&year,&month,&day);
                    if (year)
                      { 
                        tm.tm_year = year - 1900;
                        tm.tm_mon = month;
                        tm.tm_mday = day;
                        strftime(buf, sizeof(buf), "%x", &tm);
                      }
                    else 
                      snprintf(buf,sizeof(buf),"%02d/%02d",month+1,day);
                    lleft = gtk_label_new(_("Birthday:"));
                    gtk_misc_set_alignment(GTK_MISC(lleft),0,0);
                    gtk_table_attach(table, lleft, 0, 1, i, i+1, GTK_FILL, GTK_FILL, 0, 0);
                    lright = gtk_label_new(buf);
                    gtk_misc_set_alignment(GTK_MISC(lright),0,0);
                    gtk_table_attach(table, lright, 1, 2, i, i+1, GTK_FILL, GTK_FILL, 0, 0);
                    i++;
                  }
              }                
          }
      gtk_widget_show_all(GTK_WIDGET(table));
    }
  else /* show configured fields only */
    {    
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
  GtkWidget *edit_button, *delete_button;

  edit_button = g_object_get_data (o, "edit-button");
  delete_button = g_object_get_data (o, "delete-button");

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, 1, &id, -1);

      p = db_get_by_uid (id);
    
      show_details (p);

      discard_person (p);
		
      gtk_widget_set_sensitive (edit_button, TRUE);
      gtk_widget_set_sensitive (delete_button, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (edit_button, FALSE);
      gtk_widget_set_sensitive (delete_button, FALSE);
    }
}

static gboolean
match_for_search (struct person *p, const gchar *text, struct gpe_pim_category *cat)
{
  gchar *gn;
  gchar *fn;
  gchar *name;
  
  if (p->given_name)
    gn = g_utf8_strdown(p->given_name, -1);
  else
    gn = g_strdup("");
  
  if (p->family_name)
    fn = g_utf8_strdown(p->family_name, -1);
  else
    fn = g_strdup("");
  
  if (p->name)
    name = g_utf8_strdown(p->name, -1);
  else
    name = g_strdup("");
  
  if (!g_str_has_prefix(gn,text) && !g_str_has_prefix(fn,text) 
	  && !g_str_has_prefix(fn,text))
    {
      if (gn) g_free(gn);
      if (fn) g_free(fn);
      if (fn) g_free(name);
        return FALSE;
    }
  if (gn) g_free(gn);
  if (fn) g_free(fn);
  if (fn) g_free(name);
    
/* alternate method if nothing found 
  if ...
    
  else
    {
      gchar *lname = g_utf8_strdown (p->name, -1);
    
      if (strstr (lname, text) == NULL)
        {
          g_free (lname);
          return FALSE;
        }
    }

  g_free (lname);
*/
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

static gboolean
window_key_press_event (GtkWidget *widget, GdkEventKey *k, GtkTreeView *tree)
{
  GtkTreePath *path;
  
  if (k->keyval == GDK_Up)
    {
      gtk_tree_view_get_cursor(GTK_TREE_VIEW(tree),&path,NULL);
      if (path) 
        gtk_tree_path_prev(path);
      else
        path = gtk_tree_path_new_first();
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(tree),path,NULL,FALSE);
      gtk_tree_path_free(path);
      return TRUE;
    }
  if (k->keyval == GDK_Down)
    {
      gtk_tree_view_get_cursor(GTK_TREE_VIEW(tree),&path,NULL);
      if (path)
        gtk_tree_path_next(path);
      else
        path = gtk_tree_path_new_first();
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(tree),path,NULL,FALSE);
      gtk_tree_path_free(path);
      return TRUE;
    }

  if (k->keyval == GDK_Return)
    {
      edit_contact(NULL, NULL);
    }  
  return FALSE;
}

static gboolean
toolbar_key_press_event (GtkWidget *widget, GdkEventKey *k)
{
  
  if ((k->keyval == GDK_Tab) || (k->keyval == GDK_Up))
  {
    gtk_widget_grab_focus(search_entry);
    return TRUE;
  }
  return FALSE;
}

static gboolean
search_entry_key_press_event (GtkWidget *widget, GdkEventKey *k, GtkWidget *toolitem)
{
  if (k->keyval == GDK_Down)
  {
    gtk_widget_grab_focus(toolitem);
    return TRUE;
  }
  if (k->keyval == GDK_Escape)
  {
    gtk_editable_delete_text(GTK_EDITABLE(widget),0,-1);
    return TRUE;
  }
  return FALSE;
}

static gboolean
list_button_press_event (GtkWidget *widget, GdkEventButton *b, GtkListStore *list_store)
{
  if (b->button == 3)
    {
      gint x, y;
      GtkTreePath *path;
  
      x = b->x;
      y = b->y;
      
      if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
					 x, y,
					 &path, NULL,
					 NULL, NULL))
	{
	  GtkTreeIter iter;
	  guint id;

	  gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), &iter, path);

	  gtk_tree_path_free (path);

	  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 1, &id, -1);

	  menu_uid = id;

	  if (export_bluetooth_available ())
	    gtk_widget_show (bluetooth_menu_item);
	  else
	    gtk_widget_hide (bluetooth_menu_item);

	  gtk_menu_popup (GTK_MENU (popup_menu), NULL, NULL, NULL, widget, b->button, b->time);
	}
    }

  return FALSE;
}


static gboolean
list_button_release_event (GtkWidget *widget, GdkEventButton *b, GtkListStore *list_store){
  if (b->button == 3)
    return TRUE;
  
  return FALSE;
}

static GtkItemFactoryEntry popup_items[] =
{
  { "/Edit",		    NULL, menu_do_edit,           0, "<Item>" },
  { "/Delete",		    NULL, menu_do_delete,         0, "<StockItem>", GTK_STOCK_DELETE },
  { "/Save as ...",	    NULL, menu_do_save,           0, "<Item>" },
  { "/Send via Bluetooth", NULL, menu_do_send_bluetooth, 0, "<Item>", },
};

static GtkWidget *
create_popup (GtkWidget *window)
{
  GtkAccelGroup *accel_group;
  GtkItemFactory *item_factory;

  accel_group = gtk_accel_group_new ();
  item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<main>", accel_group);
  g_object_set_data_full (G_OBJECT (window), "<main>", item_factory, (GDestroyNotify) g_object_unref);
  gtk_item_factory_create_items (item_factory, sizeof (popup_items) / sizeof (popup_items[0]), 
				 popup_items, NULL);

  bluetooth_menu_item = gtk_item_factory_get_widget (item_factory, "<main>/Send via Bluetooth");

  return gtk_item_factory_get_widget (item_factory, "<main>");
}

void 
on_main_focus(GtkWindow *window,  GdkEventExpose *event,gpointer user_data)
{
  GtkWidget *searchentry = user_data;
  
  gtk_widget_grab_focus(searchentry);
  gtk_editable_select_region(GTK_EDITABLE(searchentry),0,-1);
}

static GtkWidget *
create_main (gboolean show_config_button)
{
  GtkWidget *main_window;
  GtkWidget *vbox1;
  GtkWidget *hbox1 = NULL, *hbox3;
  GtkWidget *label83, *label84;
  GtkWidget *entry1;
  GtkWidget *pDetail;
  GtkWidget *tabDetail;
  GtkWidget *toolbar, *pw;
  GtkWidget *b, *btnNew;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *tree_sel;
  GtkWidget *scrolled_window;
  gint size_x, size_y;

  /* screen layout detection */
  size_x = gdk_screen_width();
  size_y = gdk_screen_height();  
  mode_large_screen = (size_x > 240) && (size_y > 320);
  mode_landscape = (size_x > size_y);
  size_x /= 4;
  size_y /= 3;
  if (size_x < 240) size_x = 240;
  if (size_y < 320) size_y = 320;

  /* main window */
  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (main_window), _("Contacts"));
  gtk_window_set_default_size (GTK_WINDOW (main_window), size_x, size_y);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (main_window), vbox1);

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			       GTK_ORIENTATION_HORIZONTAL);

  gtk_box_pack_start (GTK_BOX (vbox1), toolbar, FALSE, FALSE, 0);

  btnNew = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_NEW,
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

  if (show_config_button)
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
  if (mode_landscape)
    {
      hbox1 = gtk_hbox_new(FALSE,gpe_get_boxspacing());
      gtk_box_pack_start (GTK_BOX (hbox1), scrolled_window, FALSE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);
    }
  else  
    {
      gtk_box_pack_start (GTK_BOX (vbox1), scrolled_window, TRUE, TRUE, 0);
    }    
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Contact"), renderer,
						     "text", 0, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

  hbox3 = gtk_hbox_new (FALSE, 0);
  if (mode_landscape)
    {
       gtk_container_set_border_width(GTK_CONTAINER(hbox3),gpe_get_border());
       gtk_box_set_spacing(GTK_BOX(hbox3),gpe_get_boxspacing());
    }
  gtk_box_pack_start (GTK_BOX (vbox1), hbox3, FALSE, FALSE, 0);

  label83 = gtk_label_new (_("Find:"));
  gtk_box_pack_start (GTK_BOX (hbox3), label83, FALSE, FALSE, 0);

  entry1 = gtk_entry_new ();
  gtk_widget_set_size_request(entry1,60,-1);
  gtk_box_pack_start (GTK_BOX (hbox3), entry1, FALSE, FALSE, 0);

  label84 = gtk_label_new (_("in"));
  gtk_box_pack_start (GTK_BOX (hbox3), label84, FALSE, FALSE, 2);

  categories_smenu = gtk_simple_menu_new ();
  gtk_box_pack_start (GTK_BOX (hbox3), categories_smenu, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (categories_smenu), "changed", 
		    G_CALLBACK (do_search), entry1);

  g_signal_connect (G_OBJECT (entry1), "activate", G_CALLBACK (do_search), entry1);
  g_signal_connect (G_OBJECT (entry1), "changed", G_CALLBACK (schedule_search), NULL);

  g_signal_connect (G_OBJECT (list_view), "button_press_event", 
		    G_CALLBACK (list_button_press_event), list_store);

  g_signal_connect (G_OBJECT (list_view), "button_release_event", 
		    G_CALLBACK (list_button_release_event), list_store);

  gtk_widget_set_events (main_window,GDK_KEY_PRESS_MASK);
  gtk_widget_set_events (entry1,GDK_KEY_PRESS_MASK);
  g_signal_connect (G_OBJECT (main_window), "key_press_event", 
		    G_CALLBACK (window_key_press_event), list_view);

  g_signal_connect (G_OBJECT (toolbar), "key_press_event", 
		    G_CALLBACK (toolbar_key_press_event), NULL);
  g_signal_connect (G_OBJECT (entry1), "key_press_event", 
		    G_CALLBACK (search_entry_key_press_event), btnNew);
            
  search_entry = entry1;

  pDetail = gtk_frame_new (_("Contact"));
  
  if (mode_landscape)
    {
      gtk_container_set_border_width(GTK_CONTAINER(pDetail),gpe_get_border());
      gtk_box_pack_start (GTK_BOX (hbox1), pDetail, TRUE, TRUE, 0);
    }
  else
    {
      gtk_box_pack_start (GTK_BOX (vbox1), pDetail, FALSE, TRUE, 0);
    }
  tabDetail = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings(GTK_TABLE(tabDetail),gpe_get_boxspacing()*2);
  gtk_container_set_border_width(GTK_CONTAINER(tabDetail),gpe_get_border());
  gtk_container_add (GTK_CONTAINER (pDetail), tabDetail);
  g_object_set_data (G_OBJECT (main_window), "tabDetail", tabDetail);
  g_object_set_data (G_OBJECT (main_window), "entry", entry1);

  g_signal_connect (G_OBJECT (tree_sel), "changed",
		    G_CALLBACK (selection_made), main_window);
 
  g_signal_connect (G_OBJECT (main_window), "focus-in-event",
		    G_CALLBACK (on_main_focus), entry1);

  displaymigration_mark_window (main_window);

  return main_window;
}

int
main (int argc, char *argv[])
{
  int arg;
  gboolean show_config_button = TRUE;
  
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

  /* check command line args */
  while ((arg = getopt(argc, argv, "n")) >= 0)
  {
    if (arg == 'n') 
      {
        show_config_button = FALSE;
        break;
      }
  }    
  
  if (db_open ())
    exit (1);

  export_init ();

  displaymigration_init ();

  load_well_known_tags ();

  list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);

  mainw = create_main (show_config_button);
  popup_menu = create_popup (mainw);
  update_categories ();
  show_details (NULL);

  gtk_signal_connect (GTK_OBJECT (mainw), "destroy", gtk_main_quit, NULL);

  load_structure ();

  /* load detail panel fields, create widgets */
  if (!mode_landscape)
    load_panel_config ();

  gpe_set_window_icon (mainw, "icon");
  gtk_widget_show_all (mainw);
  gtk_widget_grab_focus (GTK_WIDGET (g_object_get_data (G_OBJECT (mainw), "entry")));

  gtk_main ();
  return 0;
}
