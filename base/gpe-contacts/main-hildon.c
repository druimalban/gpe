/*
 * Copyright (C) 2001, 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2005, 2006, 2007, 2008 by Florian Boor <florian.boor@kernelconcepts.de>
 * Copyright (C) 2007, 2008, 2009, 2010 Graham R. Cobb <g+gpe@cobb.uk.net>
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

#include <libintl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <time.h>
#include <ctype.h>

/* GTK and GPE includes */
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/smallbox.h>
#include <gpe/errorbox.h>
#include <gpe/gtkdatecombo.h>
#include <gpe/question.h>
#include <gpe/gtksimplemenu.h>
#include <gpe/picturebutton.h>
#include <gpe/spacing.h>

#include <gpe/contacts-db.h>

#include "config.h"

/* Hildon includes */

#if HILDON_VER > 0
#include <hildon/hildon-file-chooser-dialog.h>
#include <hildon/hildon-caption.h>
#include <hildon/hildon-date-editor.h>
#include <hildon/hildon-program.h>
#include <hildon/hildon-window.h>
#if MAEMO_VERSION_MAJOR >= 5
#include <hildon/hildon-picker-button.h>
#include <hildon/hildon-touch-selector.h>
#include <hildon/hildon-button.h>
#include <hildon/hildon-entry.h>
#endif 
#else
#include <hildon-fm/hildon-widgets/hildon-file-chooser-dialog.h>
#include <hildon-widgets/hildon-caption.h>
#include <hildon-widgets/hildon-date-editor.h>
#include <hildon-widgets/hildon-input-mode-hint.h>
#include <hildon-widgets/hildon-program.h>
#include <hildon-widgets/hildon-window.h>
#include <hildon-widgets/hildon-file-chooser-dialog.h>
#endif /* HILDON_VER */

#include <libosso.h>

#include <gpe/pim-categories-ui.h>


#include "support.h"
#include "structure.h"
#include "proto.h"
#include "main.h"
#include "export.h"
#include "import-vcard.h"
#include "finddlg.h"

#define MY_PIXMAPS_DIR "gpe-contacts/"
#define ICON_PATH "/usr/share/icons/hicolor/26x26/hildon"

#define SERVICE_NAME    "gpe_contacts"
#define SERVICE_VERSION VERSION


GtkWidget *mainw;
static GtkListStore *list_store, *filter_store;
static GtkWidget *list_view;
static GtkWidget *filter_view;
#if MAEMO_VERSION_MAJOR < 5
static GtkWidget *categories_smenu;
#else
static GtkWidget *categories_picker_button;
#endif
static GtkWidget *search_entry;
static GtkWidget *popup_menu;
static GtkWidget *bluetooth_menu_item;
static GtkWidget *irda_menu_item;
#if MAEMO_VERSION_MAJOR < 5
static GtkWidget *fullscreen_control;
#endif
guint menu_uid;
gboolean mode_landscape;
gboolean mode_large_screen;
static gboolean scroll_delay = FALSE;
static gboolean do_wrap = FALSE;
static osso_context_t *context = NULL;

#define DEFAULT_STRUCTURE PREFIX "/share/gpe-contacts/default-layout.xml"
#define LARGE_STRUCTURE PREFIX "/share/gpe-contacts/default-layout-bigscreen.xml"

struct gpe_icon my_icons[] = {
  {"icon", PREFIX "/share/pixmaps/gpe-contacts.png" },
  {NULL, NULL}
};

/* the length of the filter stings is fixed to three, you need to change
   the queries in db.c to change the filter langth */

t_filter filter[]= {
  {"*"  , "***"},
  {"ABC", "abc"},
  {"DEF", "def"},      
  {"GHI", "ghi"},
  {"JKL", "jkl"},
  {"MNO", "mno"},
  {"PQR", "pqr"},
  {"STU", "stu"},
  {"VWX", "vwx"},
  {"YZ" , "yzz"},
  {"...", NULL},
};

gint num_filter = sizeof(filter) / sizeof(t_filter);
gint current_filter = 0;

static void
show_help_and_exit (void)
{
  g_print ("\nUsage: gpe-contacts [-hv] [-i <file>] [-e <file>]\n\n");
  g_print ("-h          : Show this help\n");
  g_print ("-v          : Edit owner's vcard\n");
  g_print ("-i <file>   : Import a given file and exit\n");
  g_print ("-e <file>   : Write contacts from database to vcf file using the given filename and exit\n");
  g_print ("Without command line option the GUI is launched\n\n");    
  exit (EXIT_SUCCESS);
}

#if MAEMO_VERSION_MAJOR < 5
static void
toggle_fullscreen (GtkCheckMenuItem *menuitem, gpointer user_data)
{
  if (gtk_check_menu_item_get_active (menuitem))
    gtk_window_fullscreen (GTK_WINDOW (mainw));
  else
    gtk_window_unfullscreen (GTK_WINDOW (mainw));
}
#endif

static void 
populate_filter(void)
{
  int i;
  GtkTreeIter iter;
	
  for (i=0; i<num_filter; i++)
    {
        gtk_list_store_append(GTK_LIST_STORE(filter_store), &iter);
        gtk_list_store_set (filter_store, &iter, 0, filter[i].title, 1, i, -1);
    }
}

static void
menu_do_edit (void)
{
  struct contacts_person *p;
  p = contacts_db_get_by_uid (menu_uid);
  edit_person (p,  _("Edit Contact"), FALSE);
}

static void
menu_do_delete (void)
{
  if (gpe_question_ask (_("Really delete this contact?"), _("Confirm"), 
			"question", "!gtk-cancel", NULL, "!gtk-delete", NULL, NULL) == 1)
    {
      if (contacts_db_delete_by_uid (menu_uid))
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

  table = g_object_get_data (G_OBJECT(mainw), "tabDetail");

  wlist = gtk_container_get_children (GTK_CONTAINER (table));

  for (iter = wlist; iter; iter = iter->next)
    gtk_container_remove (GTK_CONTAINER (table), GTK_WIDGET (iter->data));

  g_list_free (wlist);

  count = contacts_db_get_config_values (CONFIG_PANEL, &list);
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

  contacts_db_free_result (list);
}

static void
update_categories (void)
{
  GSList *categories = gpe_pim_categories_list (), *iter;
#if MAEMO_VERSION_MAJOR < 5
  GtkWidget *menu = categories_smenu;

  gtk_simple_menu_flush (GTK_SIMPLE_MENU (menu));

  gtk_simple_menu_append_item (GTK_SIMPLE_MENU (menu), _("All categories"));

  for (iter = categories; iter; iter = iter->next)
    {
      gint id = (gint) iter->data;
      gtk_simple_menu_append_item (GTK_SIMPLE_MENU (menu), gpe_pim_category_name(id));
    }
#else /* MAEMO_VERSION_MAJOR < 5 */
  static GtkWidget *selector = NULL;

  if (selector) gtk_widget_destroy(selector);

  /* Create a HildonTouchSelector with a single text column */
  selector = hildon_touch_selector_new_text();

  /* Attach the touch selector to the picker button*/
  hildon_picker_button_set_selector (HILDON_PICKER_BUTTON (categories_picker_button),
                                     HILDON_TOUCH_SELECTOR (selector));

  /* Set the selection mode */
  hildon_touch_selector_set_column_selection_mode (HILDON_TOUCH_SELECTOR (selector),
                                    HILDON_TOUCH_SELECTOR_SELECTION_MODE_SINGLE);

  hildon_touch_selector_append_text (HILDON_TOUCH_SELECTOR (selector),
                                       _("All categories"));


  for (iter = categories; iter; iter = iter->next)
    {
      gint id = (gint) iter->data;
     hildon_touch_selector_append_text (HILDON_TOUCH_SELECTOR (selector), gpe_pim_category_name(id));
    }
#endif /* MAEMO_VERSION_MAJOR < 5 */

  g_slist_free (categories);
}

static void
new_contact (GtkWidget * widget, gpointer d)
{
  struct contacts_person *p;
  GtkWidget *new_button, *edit_button;
	  
  new_button = g_object_get_data(G_OBJECT(mainw), "new-button");
  edit_button = g_object_get_data(G_OBJECT(mainw), "edit-button");
  gtk_widget_set_sensitive (new_button, FALSE);
  gtk_widget_set_sensitive (edit_button, FALSE);
  p = contacts_new_person ();
  edit_person (p, _("New Contact"), FALSE);
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
      struct contacts_person *p;
      GtkWidget *new_button, *edit_button;
	  
      new_button = g_object_get_data(G_OBJECT(mainw), "new-button");
      edit_button = g_object_get_data(G_OBJECT(mainw), "edit-button");
      gtk_widget_set_sensitive (new_button, FALSE);
      gtk_widget_set_sensitive (edit_button, FALSE);
      gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 1, &uid, -1);
      p = contacts_db_get_by_uid (uid);
      edit_person (p, _("Edit Contact"), FALSE);
    }
}

static void
delete_contact (GtkWidget * widget, gpointer main_window)
{
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreePath *path = NULL;

  if (!main_window) main_window = gtk_widget_get_toplevel(widget);

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      guint uid;
      GtkWidget *dialog;
        
      dialog = gtk_message_dialog_new (GTK_WINDOW(main_window),
                                       GTK_DIALOG_MODAL 
                                       | GTK_DIALOG_DESTROY_WITH_PARENT,
                                       GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
                                       _("Are you sure you want to want to "\
                                       "delete the selected contact permanently?"));
      gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                             _("OK"), GTK_RESPONSE_YES,
                             _("Cancel"), GTK_RESPONSE_NO, NULL);
        
      gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 1, &uid, -1);
      if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES)
        {
          gtk_tree_view_get_cursor(GTK_TREE_VIEW(list_view), &path, NULL);
          if (path) 
            {
              if (!gtk_tree_path_prev(path))
                gtk_tree_path_next(path);
            }
          else
            path = gtk_tree_path_new_first();
          if (contacts_db_delete_by_uid (uid))
            {
              update_display ();
              gtk_tree_view_set_cursor(GTK_TREE_VIEW(list_view), path, 
                                       NULL, FALSE);    
            }
          gtk_tree_path_free(path);
        }
      gtk_widget_destroy(dialog);
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
        if (!strcasecmp(tag,e->tag))
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
      const gchar *t = get_tag_name_1 (tag, e);
    
      if (t) 
        return g_strdup(t);
    }

  return NULL;
}

static gboolean
pop_singles (GtkWidget *vbox, GSList *list, struct contacts_person *p)
{
  if (list)
    {
      guint length = g_slist_length (list);
      GtkWidget *table = NULL;
      guint x = 0;
    
      while (list)
        {
          GSList *next = list->next;
          edit_thing_t e = list->data;
          struct contacts_tag_value *tv = NULL;
          GtkWidget *w;
          GtkWidget *l;
          gchar *name;

          if (e && e->tag) 
             tv = contacts_db_find_tag(p, e->tag);
          if (tv && tv->value)
            {
              if (table == NULL) 
                 table = gtk_table_new (length, 2, FALSE);
              w = gtk_label_new (NULL);
              gtk_label_set_selectable(GTK_LABEL(w), TRUE);
              gtk_misc_set_alignment(GTK_MISC(w),0.0,0.5);
              gtk_label_set_text(GTK_LABEL(w), tv->value);
              gtk_label_set_line_wrap(GTK_LABEL(w), TRUE);
          
              if (e && e->name && e->name[strlen(e->name)-1] != ':')
                {
                  name = g_strdup_printf("%s:", e->name);
                  l = gtk_label_new (name);
                  g_free(name);
                }
              else
                l = gtk_label_new (e->name);
              gtk_misc_set_alignment(GTK_MISC(l),0.0,0.0);
              
              gtk_table_attach (GTK_TABLE (table),
                        l,
                        0, 1, x, x + 1,
                        GTK_FILL, GTK_FILL, 0, 0);
              gtk_table_attach (GTK_TABLE (table),
                        w,
                        1, 2, x, x + 1,
                        GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                        0, 0, 0);
              x++;
            }
          g_slist_free_1 (list);
          list = next;
        }
    if (table)
      {
        gtk_table_set_col_spacings (GTK_TABLE (table), gpe_get_boxspacing());
        gtk_table_set_row_spacings (GTK_TABLE (table), gpe_get_boxspacing());
        gtk_container_set_border_width (GTK_CONTAINER (table), 0);
        gtk_widget_show_all(table);
        gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 4);
        return TRUE;
      }
    }
  return FALSE;
}

static gboolean
build_children (GtkWidget *vbox, GSList *children, struct contacts_person *p)
{
  gboolean result = FALSE;
  GSList *child;
  GSList *singles = NULL;

  for (child = children; child; child = child->next)
    {
      edit_thing_t e = child->data;
      struct contacts_tag_value *tv = NULL;
      GtkWidget *w;
      
      if (e && e->tag)
        {
          tv = contacts_db_find_tag(p, e->tag);
        }
      if (!e->hidden)
        switch (e->type)
        {
          case GROUP:
            {
              gchar *markup;
              gboolean res;
              if (e->children) /* ignore empty groups */
                {
                  markup = NULL;
                  if (vbox && singles)
                    result |= pop_singles (vbox, singles, p);
                  singles = NULL;
                  if (e->name)
                    {                      
                      markup = g_strdup_printf ("<b>%s</b>", e->name);
                      w = gtk_label_new (NULL);
                      gtk_label_set_markup (GTK_LABEL (w), markup);
                      gtk_misc_set_alignment (GTK_MISC (w), 0, 0.5);
                      g_free (markup);
                      gtk_box_pack_start (GTK_BOX (vbox), w, TRUE, TRUE, 0);
                      gtk_widget_show(w);
                        
                      res = build_children (vbox, e->children, p);
                      if (!res)
                        {
                          gtk_container_remove(GTK_CONTAINER(vbox), w);
                        }
                      result |= res;
                        
                    }
                  else 
                    result |=  build_children (vbox, e->children, p);
                }
            }
          break;
        
          case ITEM_MULTI_LINE:
          case ITEM_SINGLE_LINE:
            singles = g_slist_append (singles, e);
          break;
        
          case ITEM_DATE:
          {
            GtkWidget *l;
            GtkWidget *hbox;
            if (vbox && singles)
              result |= pop_singles (vbox, singles, p);
            singles = NULL;
            if (tv && tv->value)
              {
                int year,month,day;
                struct tm tm;
                char buf[32];
                
                result = TRUE;
                l = gtk_label_new (e->name);
                gtk_misc_set_alignment(GTK_MISC(l), 0.0, 0.5);
                memset(&tm, 0, sizeof(tm));
                sscanf(tv->value,"%04d%02d%02d", &year, &month, &day);
                if (year)
                  { 
                    tm.tm_year = year - 1900;
                    tm.tm_mon = month - 1;
                    tm.tm_mday = day;
                    strftime(buf, sizeof(buf), "%x", &tm);
                  }
                else 
                  snprintf(buf,sizeof(buf),"%02d/%02d", month, day);
                w = gtk_label_new(buf);
                gtk_misc_set_alignment(GTK_MISC(w), 0.0, 0.5);
                hbox = gtk_hbox_new(FALSE, gpe_get_boxspacing());
                gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, TRUE, 0);
                gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, TRUE, 0);
                gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
                result = TRUE;
              }
          }
          break;
          case ITEM_IMAGE:
          {
            GtkWidget *l = gtk_label_new (e->name);
            GtkWidget *hbox, *image;
            GdkPixbuf *buf;
            
            if (!(tv && tv->value))
              continue;
            if (vbox && singles)
              pop_singles (vbox, singles, p);
            singles = NULL;
            result = TRUE;
            hbox = gtk_hbox_new (FALSE, gpe_get_boxspacing());
            gtk_misc_set_alignment (GTK_MISC (l), 0.0, 0.0);
            gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, TRUE, 0);
  
            image = gtk_image_new();
            gtk_widget_set_size_request(image, IMG_WIDTH, IMG_HEIGHT);
            buf = gdk_pixbuf_new_from_file_at_size(tv->value, IMG_WIDTH, 
                                          IMG_HEIGHT, NULL);
            gtk_image_set_from_pixbuf(GTK_IMAGE(image), buf);
            gdk_pixbuf_unref(buf);
            gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, TRUE, 0);
            gtk_widget_show_all(hbox);
            gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
          }
          default:
            /* just ignore */
          break;
        }
    }

  if (vbox && singles)
    result |= pop_singles (vbox, singles, p);
  singles = NULL;
  
  return result;
}

static int
show_details (struct contacts_person *p)
{
  GtkWidget *lleft, *lright;
  GtkTable *table;
  gint i;
  gpointer pchild;
  gchar *tagname;
  struct contacts_tag_value *curtag;
  GList *wlist, *witer;
  GSList *page;
  GtkWidget *label;
  gchar *text;

  table = GTK_TABLE (g_object_get_data(G_OBJECT(mainw), "tabDetail"));
  
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
      if (p != NULL) /* add contact data */
        {
          GtkWidget *vbox = gtk_vbox_new(FALSE, gpe_get_boxspacing());
          gchar *catstring = build_categories_string(p);
        
          gtk_table_attach(GTK_TABLE(table), vbox, 0, 1, 0, 1, 
                           GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
          /* we use data layout to structure widgets */
          for (page = edit_pages; page; page = page->next) 
            {
               edit_thing_t e = page->data;
               /* add section content */
               if (!e->hidden)
                 build_children (vbox, e->children, p);
            }
          if (catstring)
            {
              text = g_strdup_printf("%s: %s", _("Categories"), catstring);
              label = gtk_label_new(NULL);
              gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
              gtk_label_set_markup(GTK_LABEL(label), text);
              gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, 
                               GTK_FILL, GTK_FILL, 0, 0);
              g_free(text);
            }
        }
      else  /* just add some feedback */
        {
          text = g_strdup_printf("<b>%s</b>", _("No contact selected."));
          label = gtk_label_new(NULL);
          gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
          gtk_label_set_markup(GTK_LABEL(label), text);
          gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, 
                           GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
          g_free(text);
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
                contacts_db_get_config_tag (CONFIG_PANEL,
                           g_strdup (gtk_label_get_text(GTK_LABEL (lleft))));
              curtag = contacts_db_find_tag (p, tagname);
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

static void
show_details_window(GtkWidget *btn)
{
  GtkWidget *dlgDetail;
  GtkWidget *button, *label;
  GtkWidget *vbox, *vport, *scrolled_window;
  gchar *catstring, *text;
  GtkTreeIter iter;
  GtkTreeSelection *sel;
  GtkTreeModel *model;
  GSList *page;
  struct contacts_person *p;
  
  /* get person data */
  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list_view));
  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      gint id;
      gtk_tree_model_get (model, &iter, 1, &id, -1);
      p = contacts_db_get_by_uid (id);
    }
  else
    return;
  
  /* create and set up dialog */
  dlgDetail = gtk_dialog_new_with_buttons(_("Contact"), GTK_WINDOW(mainw), 
                                          GTK_DIALOG_MODAL 
                                          | GTK_DIALOG_DESTROY_WITH_PARENT,
                                          NULL);
  gtk_window_set_default_size(GTK_WINDOW(dlgDetail), 220, 300);
  button = gtk_dialog_add_button(GTK_DIALOG(dlgDetail), 
                                 GTK_STOCK_CLOSE, 
                                 GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(button);
  
  vbox = gtk_vbox_new(FALSE, gpe_get_boxspacing());
  scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
  vport = gtk_viewport_new(NULL, NULL);
  gtk_viewport_set_shadow_type(GTK_VIEWPORT(vport), GTK_SHADOW_NONE);
  gtk_container_add(GTK_CONTAINER(vport), vbox);
  gtk_container_add (GTK_CONTAINER (scrolled_window), vport);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlgDetail)->vbox), 
                     scrolled_window, TRUE, TRUE, 0);
  
  /* create content */
  catstring = build_categories_string(p);
    
  for (page = edit_pages; page; page = page->next) /* we use data layout to structure widgets */
    {
       edit_thing_t e = page->data;

      if ((!e->hidden) && (e->children))
        {
          text = g_strdup_printf ("<b>%s</b>", e->name);
          label = gtk_label_new (NULL);
          gtk_label_set_markup (GTK_LABEL (label), text);
          gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
          g_free (text);
          gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
          if (!build_children (vbox, e->children, p))
            gtk_container_remove(GTK_CONTAINER(vbox), label);
        }
    }
  if (catstring)
    {
      text = g_strdup_printf("%s: %s", _("Categories"), catstring);
      label = gtk_label_new(NULL);
      gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
      gtk_label_set_markup(GTK_LABEL(label), text);
      gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
      g_free(text);
    }

  /* show details dialog */
  gtk_widget_show_all(dlgDetail);
  gtk_dialog_run(GTK_DIALOG(dlgDetail));
  gtk_widget_destroy(dlgDetail);
  contacts_discard_person (p);
}

static void
selection_made (GtkTreeSelection *sel, GObject *o)
{
  GtkTreeIter iter;
  guint id;
  struct contacts_person *p;
  GtkTreeModel *model;
  GtkWidget *edit_button, *delete_button, *new_button;

  delete_button = g_object_get_data (o, "delete-button");
  edit_button = g_object_get_data (o, "edit-button");
  new_button = g_object_get_data (o, "new-button");
  
  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, 1, &id, -1);

      p = contacts_db_get_by_uid (id);
      show_details (p);
      contacts_discard_person (p);
		
      gtk_widget_set_sensitive (delete_button, TRUE);
      gtk_widget_set_sensitive (edit_button, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (delete_button, FALSE);
      gtk_widget_set_sensitive (edit_button, FALSE);
    }
	/* restore after edit/new */
    gtk_widget_set_sensitive (new_button, TRUE);
}

static gboolean
match_for_search (struct contacts_person *p, const gchar *text, gint catid)
{
  gchar *fn;
  gchar *name;
  gchar *company;
  if ((text == NULL) && (catid == -1)) /* some speedup */
    return TRUE;
  
  if (text)
    {
      if (p->family_name)
        fn = g_utf8_strdown(p->family_name, -1);
      else
        fn = g_strdup("");
      
      if ((p->name) && strlen(p->name))
        name = g_utf8_strdown(p->name, -1);
      else
        name = g_strdup("");
      
      if (p->company)
        company = g_utf8_strdown(p->company, -1);
      else
        company = g_strdup("");
      
      if (!g_str_has_prefix(fn, text) 
          && !g_str_has_prefix(name, text)
          && !g_str_has_prefix(company, text))
        {
          if (fn) g_free(fn);
          if (name) g_free(name);
          if (company) g_free(company);
          return FALSE;
        }
      if (fn) g_free(fn);
      if (name) g_free(name);
      if (company) g_free(company);
    }

  if (catid != -1)
    {
      GSList *l;
      gboolean found = FALSE;
      struct contacts_person *per = contacts_db_get_by_uid (p->id);
      for (l = per->data; l; l = l->next)
        {
          struct contacts_tag_value *v = l->data;
          if (!strcasecmp (v->tag, "CATEGORY") && v->value)
            {
              guint c = atoi (v->value);
              if (c == catid)
                {
                  found = TRUE;
                  break;
                }
            }
        }
      contacts_discard_person(per);
      if (!found)
        return FALSE;
    }

  return TRUE;
}

static void
do_search (GObject *obj, GtkWidget *entry)
{
  gchar *cat_id = NULL;
  GSList *sel_entries = NULL, *iter = NULL;
  GtkTreePath *path;

#if MAEMO_VERSION_MAJOR < 5
  gchar *text = g_utf8_strdown (gtk_entry_get_text (GTK_ENTRY (entry)), -1);
  guint category = gtk_option_menu_get_history (GTK_OPTION_MENU (categories_smenu));
  if (category)
    {
      GSList *l = gpe_pim_categories_list ();
      GSList *ll = g_slist_nth (l, category - 1);

      if (ll) 
        {
          cat_id = g_strdup_printf("%u", (gint)ll->data);
        }

      g_slist_free (l);
    }
#else
  gchar *text = g_utf8_strdown (hildon_entry_get_text (HILDON_ENTRY (entry)), -1);
  gint category = gpe_pim_category_id (hildon_button_get_value (HILDON_BUTTON (categories_picker_button)));
  if (category >= 0) {
    cat_id = g_strdup_printf("%u", category);
  }
#endif

  sel_entries = contacts_db_get_entries_list_filtered (text, filter[current_filter].list, cat_id);
    
  if (cat_id) 
    g_free(cat_id);
    
  sel_entries = g_slist_sort (sel_entries, (GCompareFunc)contacts_sort_entries);

  gtk_list_store_clear (list_store);
  show_details(NULL);

  for (iter = sel_entries; iter; iter = iter->next)
    {
      struct contacts_person *p = iter->data;
      GtkTreeIter titer;
      
      gtk_list_store_append (list_store, &titer);
      gtk_list_store_set (list_store, &titer, 0, 
                          (p->name[0] != '(') ? p->name : p->company, 1, p->id, -1);
      contacts_discard_person (p);
    }
  
  g_slist_free (sel_entries);
  if (text)
    g_free (text);
  
  /* select first */
  path = gtk_tree_path_new_first();
  gtk_tree_view_set_cursor(GTK_TREE_VIEW(list_view), path, NULL, FALSE);
  gtk_tree_path_free(path);
}

static void
do_find (GtkWidget *widget, gpointer main_window)
{
  gchar *cat_id = NULL;
  GSList *all_entries = NULL, *iter = NULL;
  gint c = -1;
  GtkTreePath *path;

  if (!main_window) main_window = gtk_widget_get_toplevel(widget);

#if MAEMO_VERSION_MAJOR < 5
  guint category = gtk_option_menu_get_history (GTK_OPTION_MENU (categories_smenu));
  if (category)
    {
      GSList *l = gpe_pim_categories_list ();
      GSList *ll = g_slist_nth (l, category - 1);

      if (ll)
        {
	  c = (gint)ll->data;
          cat_id = g_strdup_printf("%u", c);
        }

      g_slist_free (l);
    }
#else
  gint category = gpe_pim_category_id (hildon_button_get_value (HILDON_BUTTON (categories_picker_button)));
  if (category >= 0) {
    cat_id = g_strdup_printf("%u", category);
  }
#endif

  all_entries = do_find_contacts(GTK_WINDOW(main_window), cat_id);
  if (cat_id) 
    g_free(cat_id);
  if (!all_entries)
    return;
  all_entries = g_slist_sort (all_entries, (GCompareFunc)contacts_sort_entries);

  gtk_list_store_clear (list_store);
  show_details(NULL);

  for (iter = all_entries; iter; iter = iter->next)
    {
      struct contacts_person *p = iter->data;
      GtkTreeIter titer;
      
      if (match_for_search (p, NULL, c))
        {
	      gtk_list_store_append (list_store, &titer);
          gtk_list_store_set (list_store, &titer, 0, 
                              (p->name[0] != '(') ? p->name : p->company, 1, p->id, -1);
        }

      contacts_discard_person (p);
    }
  
  g_slist_free (all_entries);
  
  /* select first */
  path = gtk_tree_path_new_first();
  gtk_tree_view_set_cursor(GTK_TREE_VIEW(list_view), path, NULL, FALSE);
  gtk_tree_path_free(path);
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
  GtkTreeIter iter;
  GtkTreePath *path = NULL;
  GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list_view));

  gtk_tree_selection_get_selected (sel, NULL, &iter);
  path = gtk_tree_model_get_path (gtk_tree_view_get_model(GTK_TREE_VIEW(list_view)), &iter);
  update_categories ();
  do_search (G_OBJECT (search_entry), search_entry);
  if (path)
    {
      gtk_tree_selection_select_path (sel, path);
      gtk_tree_path_free (path);
    }
  selection_made(sel, G_OBJECT(mainw));
}

static gboolean
scroll_timeout(gpointer d)
{
  do_wrap = TRUE;
  scroll_delay = FALSE;
  return FALSE;
}

void
list_view_cursor_changed (GtkTreeView *treeview, gpointer user_data)
{
  GtkWidget *lStatus = user_data;
  int pos;
  int count =
      gtk_tree_model_iter_n_children(GTK_TREE_MODEL(list_store), NULL);
  GtkTreePath *path;
  
  if (lStatus)
    {
      gtk_tree_view_get_cursor(treeview, &path, NULL);
      if (path)
        {
          gchar *text;
          gint *index = gtk_tree_path_get_indices(path);
          if (index)
            {
              pos = index[0] + 1;
              text = g_strdup_printf("%d / %d", pos, count);
              gtk_label_set_text(GTK_LABEL(lStatus), text);
              g_free(text);
            }
          gtk_tree_path_free(path);
        }
    }
}

static gboolean
window_key_press_event (GtkWidget *widget, GdkEventKey *k, GtkTreeView *tree)
{
  GtkTreePath *path;
  
  /* common hotkeys */
  if (k->state & GDK_CONTROL_MASK)
    {
        switch (k->keyval)
          {
            case GDK_f:
              do_find(widget, NULL);
              return TRUE;
            break;
            case GDK_q:
              gtk_main_quit();
              return TRUE;
            break;
            case GDK_n:
              new_contact(widget, NULL);
              return TRUE;
            break;
            case GDK_d:
              delete_contact(widget, NULL);
              return TRUE;
            break;
          }
    }
  
  if (k->keyval == GDK_Up)
    {
      gtk_tree_view_get_cursor(GTK_TREE_VIEW(tree),&path,NULL);
      if (path) 
      {
        GtkTreePath *p;
        int count =
          gtk_tree_model_iter_n_children(GTK_TREE_MODEL(list_store), NULL); /* count */
        p = gtk_tree_path_copy(path);
        gtk_tree_path_prev(path);
        if (!gtk_tree_path_compare(path, p))  /* same? wrap around */
        {
          if (do_wrap)
            {
              path = gtk_tree_path_new_from_indices(count - 1, -1);
              do_wrap = FALSE;
            }
          else
            if (!scroll_delay)
              {
                g_timeout_add(300, scroll_timeout, NULL);
                scroll_delay = TRUE;
              }
        }
        gtk_tree_path_free(p);  
      }
      else
        path = gtk_tree_path_new_first();
      
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(tree), path, NULL, FALSE);
      gtk_tree_path_free(path);
      do_wrap = FALSE;
      return TRUE;
    }
  if (k->keyval == GDK_Down)
    {
      gtk_tree_view_get_cursor(GTK_TREE_VIEW(tree), &path, NULL);
      if (path)
        gtk_tree_path_next(path);
      else /* wrap around */
        {    
          if (do_wrap)
            {
              path = gtk_tree_path_new_first();
              do_wrap = FALSE;
            }
          else
            if (!scroll_delay)
              {
                g_timeout_add(400, scroll_timeout, NULL);
                scroll_delay = TRUE;
              }
        }
      if (path)
        {
          gtk_tree_view_set_cursor(GTK_TREE_VIEW(tree), path, NULL, FALSE);
          gtk_tree_path_free(path);
        }
      do_wrap = FALSE;
      return TRUE;
    }

  if ((k->keyval == GDK_Delete) 
      && (!GTK_WIDGET_HAS_FOCUS(search_entry) 
          || !strlen(gtk_entry_get_text(GTK_ENTRY(search_entry)))))
    {
      delete_contact(widget, NULL);
      return TRUE;
    }
  
  if (k->keyval == GDK_Return)
    {
      edit_contact(NULL, NULL);
      return TRUE;
    }

#if MAEMO_VERSION_MAJOR < 5
  /* toggle fullscreen */
  if (k->keyval == GDK_F6)
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (fullscreen_control), 
             !gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (fullscreen_control)));
#endif
  
  /* automatic search */
  if (!k->state && k->string && isalpha(k->string[0]))
    {
      if (!GTK_WIDGET_HAS_FOCUS(search_entry))
        gtk_widget_grab_focus(search_entry);
      return FALSE;
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
    gtk_entry_set_text(GTK_ENTRY(widget), " ");
    gtk_editable_delete_text(GTK_EDITABLE(widget), 0, -1);
    return TRUE;
  }
  return FALSE;
}

static gboolean
list_button_press_event (GtkWidget *widget, GdkEventButton *b, GtkListStore *list_store)
{
  if ((b->button == 3) || ((b->button == 1) && (b->type == GDK_2BUTTON_PRESS)))
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
          
          if (b->button == 3)
            {
              gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 1, &id, -1);
          
              menu_uid = id;
          
              if (export_bluetooth_available ())
                gtk_widget_show (bluetooth_menu_item);
              else
                gtk_widget_hide (bluetooth_menu_item);
              
              if (export_irda_available ())
                gtk_widget_show (irda_menu_item);
              else
                gtk_widget_hide (irda_menu_item);
              
              gtk_menu_popup (GTK_MENU (popup_menu), 
                              NULL, NULL, NULL, widget, b->button, b->time);
            }
          else
            {
                if (mode_landscape)
                  edit_contact(widget, NULL);
                else
                  show_details_window(widget);
            }
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

static void
filter_selected (GtkTreeSelection *sel, GObject *o)
{
  GtkTreeIter iter;
  gint id;
  GtkTreeModel *model;

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, 1, &id, -1);
      current_filter = id;
      update_display();
    }
}

static GtkItemFactoryEntry popup_items[] =
{
  { "/Edit",		    NULL, menu_do_edit,           0, "<Item>" },
  { "/Delete",		    NULL, menu_do_delete,         0, "<StockItem>", GTK_STOCK_DELETE },
  { "/Save as ...",	    NULL, menu_do_save,           0, "<Item>" },
  { "/Send via Bluetooth", NULL, menu_do_send_bluetooth, 0, "<Item>", },
  { "/Send via Irda", NULL, menu_do_send_irda, 0, "<Item>", },
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
  irda_menu_item = gtk_item_factory_get_widget (item_factory, "<main>/Send via Irda");

  return gtk_item_factory_get_widget (item_factory, "<main>");
}

#if MAEMO_VERSION_MAJOR < 5
void 
on_main_focus(GtkWindow *window,  GdkEventExpose *event,gpointer user_data)
{
  GtkWidget *searchentry = user_data;
  
  gtk_widget_grab_focus(searchentry);
  gtk_editable_select_region(GTK_EDITABLE(searchentry),0,-1);
}
#endif

static int 
import_one_file(const gchar *filename, GError **error)
{
  int result = 0;
	
  result = import_vcard(filename, error);

  return result;
}

static int 
export_one_file(const gchar *filename, GError **error)
{
  int result;

  result = contacts_db_open (FALSE);
  if (result != 0) {
    g_set_error(error, GPECONTACT_IMPEXPORT_ERROR, GPECONTACT_IMPEXPORT_ERROR_DBOPEN, GPECONTACT_IMPEXPORT_ERROR_DBOPEN_STR);
    return result;
  }
	
  result = export_db(filename, error);

  contacts_db_close();

  return result;
}

static void
on_import_vcard (GtkWidget *widget, gpointer main_window)
{
  GtkWidget *filesel, *feedbackdlg;

  if (!main_window) main_window = gtk_widget_get_toplevel(widget);
  
  filesel = hildon_file_chooser_dialog_new(GTK_WINDOW(main_window), 
                                                      GTK_FILE_CHOOSER_ACTION_OPEN);
  gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(filesel), TRUE);
  
  if (gtk_dialog_run(GTK_DIALOG(filesel)) == GTK_RESPONSE_OK)
    {
      GError *error = NULL;
      gchar *errstr = NULL;
      gint ec = 0;
      GSList *f;
      GSList *files = 
        gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(filesel));
      gtk_widget_hide(filesel);
      f = files;
      while (f && f->data)
        {
          if (import_one_file(f->data, &error) < 0) 
            {
              gchar *tmp;
              if (!errstr) 
                errstr=g_strdup("");
              ec++;
              tmp = g_strdup_printf("%s\n%s: %s", errstr, 
                                    strrchr(f->data,'/') + 1, error->message);
              if (errstr) 
                 g_free(errstr);
              errstr = tmp;
              g_clear_error(&error);
            }
          f = f->next;
        }
      if (ec)
        feedbackdlg = gtk_message_dialog_new(GTK_WINDOW(main_window),
          GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, 
          _("Import of %i files failed:\n%s"), ec, errstr);
      else
        feedbackdlg = gtk_message_dialog_new(GTK_WINDOW(main_window),
          GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, 
          _("Import successful"));
      gtk_dialog_run(GTK_DIALOG(feedbackdlg));
      gtk_widget_destroy(feedbackdlg);
    }
  gtk_widget_destroy(filesel);
  contacts_db_open(FALSE);
  update_display();
}

#ifdef IS_HILDON
static void
toggle_toolbar(GtkCheckMenuItem *menuitem, gpointer user_data)
{
  GtkWidget *toolbar = g_object_get_data(G_OBJECT(mainw), "toolbar");
  
  if (!toolbar) 
	return;
	  
  if (gtk_check_menu_item_get_active(menuitem))
    gtk_widget_show(toolbar);
  else
    gtk_widget_hide(toolbar);
}

static void
edit_categories (GtkWidget *w, gpointer main_window)
{
  GtkWidget *dialog;

  if (!main_window) main_window = gtk_widget_get_toplevel(w);

  dialog = gpe_pim_categories_dialog (NULL, FALSE, 
                                      G_CALLBACK(update_categories), NULL);
  gtk_window_set_transient_for(GTK_WINDOW(dialog), 
                               GTK_WINDOW(main_window));
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
}

static void
on_export_vcard (GtkWidget *widget, gpointer main_window)
{
  GtkWidget *filesel, *feedbackdlg;
  gint i = 0, ec = 0;
  GtkTreeIter iter;
  gchar *filename = g_strconcat(g_get_home_dir(), "GPE.vcf", NULL);

  if (!main_window) main_window = gtk_widget_get_toplevel(widget);
    
  filesel = hildon_file_chooser_dialog_new(GTK_WINDOW(main_window), 
                                                      GTK_FILE_CHOOSER_ACTION_SAVE);
  gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(filesel), TRUE);
  gtk_file_chooser_set_filename (GTK_FILE_CHOOSER(filesel), filename);
  gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(filesel), FALSE);
  
  if (gtk_dialog_run(GTK_DIALOG(filesel)) == GTK_RESPONSE_OK)
    {
      guint uid;
      gchar *filename = 
        gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filesel));
      gtk_widget_hide(filesel);
      
      gtk_tree_model_get_iter_first (GTK_TREE_MODEL(list_store), &iter);
      do 
        {
          gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 1, &uid, -1);
          if (save_to_file (uid, filename, i ? TRUE : FALSE))
             ec++;
          i++;
        }
      while (gtk_tree_model_iter_next (GTK_TREE_MODEL(list_store), &iter));
        
      if (ec)
        feedbackdlg = gtk_message_dialog_new(GTK_WINDOW(mainw),
          GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, 
          _("Export of %i files OK.  Failed: %i"), i - ec, ec);
      else
        feedbackdlg = gtk_message_dialog_new(GTK_WINDOW(mainw),
          GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, 
          _("Export successful"));
      gtk_dialog_run(GTK_DIALOG(feedbackdlg));
      gtk_widget_destroy(feedbackdlg);
    }
  g_free(filename);
  gtk_widget_destroy(filesel);
}

/* create hildon application main menu */
static void
create_app_menu(HildonWindow *window)
{
#if MAEMO_VERSION_MAJOR >= 5
  /* Use Fremantle HildonAppMenu */
  HildonAppMenu *menu_main;
  GtkWidget *button;

  menu_main = HILDON_APP_MENU (hildon_app_menu_new ());

  button = gtk_button_new_with_label (_("Find contact"));
  g_signal_connect_after (button, "clicked", G_CALLBACK (do_find), window);
  hildon_app_menu_append (menu_main, GTK_BUTTON (button));

  button = gtk_button_new_with_label (_("Import VCard"));
  g_signal_connect_after (button, "clicked", G_CALLBACK (on_import_vcard), window);
  hildon_app_menu_append (menu_main, GTK_BUTTON (button));

  button = gtk_button_new_with_label (_("Export listed contacts"));
  g_signal_connect_after (button, "clicked", G_CALLBACK (on_export_vcard), window);
  hildon_app_menu_append (menu_main, GTK_BUTTON (button));

  button = gtk_button_new_with_label (_("Categories"));
  g_signal_connect_after (button, "clicked", G_CALLBACK (edit_categories), window);
  hildon_app_menu_append (menu_main, GTK_BUTTON (button));

  gtk_widget_show_all (GTK_WIDGET (menu_main));
  hildon_window_set_app_menu (HILDON_WINDOW (window), menu_main);

#else /* MAEMO_VERSION_MAJOR >= 5 */

  /* Use traditional menu */
  GtkMenu   *menu_main = GTK_MENU (gtk_menu_new());
  GtkWidget *menu_contacts = gtk_menu_new();
  GtkWidget *menu_categories = gtk_menu_new();
  GtkWidget *menu_view = gtk_menu_new();
  GtkWidget *menu_tools = gtk_menu_new();
    
  GtkWidget *item_contacts = gtk_menu_item_new_with_label(_("Contact"));
  GtkWidget *item_categories = gtk_menu_item_new_with_label(_("Categories"));
  GtkWidget *item_close = gtk_menu_item_new_with_label(_("Close"));
  GtkWidget *item_open = gtk_menu_item_new_with_label(_("Open..."));
  GtkWidget *item_add = gtk_menu_item_new_with_label(_("Add new"));
  GtkWidget *item_delete = gtk_menu_item_new_with_label(_("Delete"));
  GtkWidget *item_catedit = gtk_menu_item_new_with_label(_("Edit categories"));
  GtkWidget *item_view = gtk_menu_item_new_with_label(_("View"));
  GtkWidget *item_fullscreen = gtk_check_menu_item_new_with_label(_("Fullscreen"));
  GtkWidget *item_toolbar = gtk_check_menu_item_new_with_label(_("Show toolbar"));
  GtkWidget *item_tools = gtk_menu_item_new_with_label(_("Tools"));
  GtkWidget *item_find = gtk_menu_item_new_with_label(_("Find contact..."));
  GtkWidget *item_import = gtk_menu_item_new_with_label(_("Import VCard"));
  GtkWidget *item_export = gtk_menu_item_new_with_label(_("Export listed contacts..."));

  gtk_menu_append (GTK_MENU(menu_contacts), item_open);
  gtk_menu_append (GTK_MENU(menu_contacts), item_add);
  gtk_menu_append (GTK_MENU(menu_contacts), item_delete);
  gtk_menu_append (GTK_MENU(menu_categories), item_catedit);
  gtk_menu_append (GTK_MENU(menu_view), item_fullscreen);
  gtk_menu_append (GTK_MENU(menu_view), item_toolbar);
  gtk_menu_append (menu_main, item_contacts);
  gtk_menu_append (menu_main, item_categories);
  gtk_menu_append (menu_main, item_view);
  gtk_menu_append (menu_main, item_tools);
  gtk_menu_append (menu_main, item_close);
  gtk_menu_append (menu_tools, item_find);
  gtk_menu_append (menu_tools, item_import);
  gtk_menu_append (menu_tools, item_export);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item_contacts), menu_contacts);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item_categories), menu_categories);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item_view), menu_view);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item_fullscreen), FALSE);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item_toolbar), TRUE);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item_tools), menu_tools);
  

  g_signal_connect(G_OBJECT(item_add), "activate", G_CALLBACK(new_contact), window);
  g_signal_connect(G_OBJECT(item_open), "activate", G_CALLBACK(edit_contact), window);
  g_signal_connect(G_OBJECT(item_catedit), "activate", G_CALLBACK(edit_categories), window);
  g_signal_connect(G_OBJECT(item_fullscreen), "activate", G_CALLBACK(toggle_fullscreen), window);
  g_signal_connect(G_OBJECT(item_toolbar), "activate", G_CALLBACK(toggle_toolbar), window);
  g_signal_connect(G_OBJECT(item_delete), "activate", G_CALLBACK(delete_contact), window);
  g_signal_connect(G_OBJECT(item_import), "activate", G_CALLBACK(on_import_vcard), window);
  g_signal_connect(G_OBJECT(item_export), "activate", G_CALLBACK(on_export_vcard), window);
  g_signal_connect(G_OBJECT(item_close), "activate", G_CALLBACK(gtk_main_quit), window);
  g_signal_connect(G_OBJECT(item_find), "activate", G_CALLBACK(do_find), window);

  gtk_widget_show_all (GTK_WIDGET(menu_main));  
  fullscreen_control = item_fullscreen;
  hildon_window_set_menu (HILDON_WINDOW (window), menu_main);
#endif /* MAEMO_VERSION_MAJOR >= 5 */
}
#endif

static GtkWidget *
create_main (gboolean edit_structure)
{
  GtkWidget *main_window;
  GtkWidget *vbox1;
  GtkWidget *hbox1 = NULL;
  GtkWidget *label83, *label84;
  GtkWidget *entry1;
  GtkWidget *pDetail;
  GtkWidget *tabDetail;
  GtkWidget *toolbar;
  GtkWidget *btnNew;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkWidget *filter_window;
  GtkTreeSelection *filter_sel;
  GtkTreeSelection *tree_sel;
  GtkWidget *scrolled_window;
  GtkWidget *lStatus = NULL;
  gint size_x, size_y;
	
  GtkWidget *w;
  GtkToolItem *item;
  HildonProgram *app;

  /* screen layout detection */
  size_x = gdk_screen_width();
  size_y = gdk_screen_height();  
  size_x /= 4;
  size_y /= 3;
  if (size_x < 240) size_x = 240;
  if (size_y < 320) size_y = 320;

  /* main application and appview */
  app = HILDON_PROGRAM ( hildon_program_get_instance () );
  g_set_application_name ( _("GPE Contacts") );
  main_window = hildon_window_new();
#if MAEMO_VERSION_MAJOR < 5
  gtk_window_set_title (GTK_WINDOW (main_window), _("List"));
#else
  gtk_window_set_title (GTK_WINDOW (main_window), _("GPE Contacts"));
#endif
  hildon_program_add_window (app, HILDON_WINDOW (main_window));
  gtk_widget_show_all (main_window);

  create_app_menu (HILDON_WINDOW(main_window));
  
  vbox1 = gtk_vbox_new (FALSE, gpe_get_boxspacing());
  gtk_container_add (GTK_CONTAINER (main_window), vbox1);

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			       GTK_ORIENTATION_HORIZONTAL);

  /* toolbar */
  hildon_window_add_toolbar(HILDON_WINDOW(main_window), GTK_TOOLBAR(toolbar));
  
  /* buttons left */
  item = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
  g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(new_contact), NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 0);
  g_object_set_data (G_OBJECT (main_window), "new-button", item);
  btnNew = GTK_WIDGET(item);
  
  item = gtk_tool_button_new_from_stock(GTK_STOCK_EDIT);
  g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(edit_contact), NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  g_object_set_data (G_OBJECT (main_window), "edit-button", item);
  gtk_widget_set_sensitive(GTK_WIDGET(item), FALSE);
  
  item = gtk_tool_button_new_from_stock(GTK_STOCK_DELETE);
  g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(delete_contact), NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  g_object_set_data (G_OBJECT (main_window), "delete-button", item);
  gtk_widget_set_sensitive(GTK_WIDGET(item), FALSE);
  
  w = gtk_image_new_from_stock(GTK_STOCK_INDEX,
			       gtk_toolbar_get_icon_size(GTK_TOOLBAR (toolbar)));
  item = gtk_tool_button_new(w, _("Categories"));
  g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK (edit_categories), NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

  /* "find" section of toolbar */
  item = gtk_separator_tool_item_new();
  gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

  label83 = gtk_label_new (_("Find:"));
  item = gtk_tool_item_new();
  gtk_container_add(GTK_CONTAINER(item), label83);
  gtk_tool_item_set_expand(GTK_TOOL_ITEM(item), FALSE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

#if MAEMO_VERSION_MAJOR < 5
  item = gtk_separator_tool_item_new();
  gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  
  entry1 = gtk_entry_new ();
  gtk_widget_set_size_request(entry1, 120, -1);
#else
  entry1 = hildon_entry_new (HILDON_SIZE_AUTO);
#endif
  item = gtk_tool_item_new();
  gtk_container_add(GTK_CONTAINER(item), entry1);
  gtk_tool_item_set_expand(GTK_TOOL_ITEM(item), FALSE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

  item = gtk_separator_tool_item_new();
  gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  
  label84 = gtk_label_new (_("in"));
  item = gtk_tool_item_new();
  gtk_container_add(GTK_CONTAINER(item), label84);
  gtk_tool_item_set_expand(GTK_TOOL_ITEM(item), FALSE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

  item = gtk_separator_tool_item_new();
  gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  
  item = gtk_tool_item_new();
#if MAEMO_VERSION_MAJOR < 5
  categories_smenu = gtk_simple_menu_new ();
  gtk_container_add(GTK_CONTAINER(item), categories_smenu);
#else
  /* Create a picker button */
  categories_picker_button = hildon_picker_button_new (HILDON_SIZE_AUTO,
                                            HILDON_BUTTON_ARRANGEMENT_VERTICAL);
  /* Set a title to the button */
  hildon_button_set_title (HILDON_BUTTON (categories_picker_button), _("Category"));

  gtk_container_add(GTK_CONTAINER(item), categories_picker_button);
#endif
  gtk_tool_item_set_expand(GTK_TOOL_ITEM(item), TRUE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

  /* filter list */
  filter_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request(filter_window, 60, -1);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (filter_window), 
                                  GTK_POLICY_NEVER, GTK_POLICY_NEVER);
  filter_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (filter_store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (filter_view), FALSE);
  filter_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (filter_view));
  gtk_tree_selection_set_mode(filter_sel, GTK_SELECTION_BROWSE);

  GTK_WIDGET_UNSET_FLAGS (filter_view, GTK_CAN_FOCUS);
  gtk_container_add (GTK_CONTAINER (filter_window), filter_view);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL, renderer,
						                             "text", 0, NULL);
  if (mode_landscape)
    {
      /* min width = 5% */
      gtk_tree_view_column_set_min_width(column, size_x / 20);
      /* max width = 10% */
      gtk_tree_view_column_set_max_width(column, size_x / 10); 
    }
  gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (filter_view), column);


  /* contacts list */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request(scrolled_window, 220, -1);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), 
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  list_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list_view), FALSE);
  tree_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
  gtk_tree_selection_set_mode(tree_sel, GTK_SELECTION_BROWSE);

  GTK_WIDGET_UNSET_FLAGS (list_view, GTK_CAN_FOCUS);
  
  gtk_container_add (GTK_CONTAINER (scrolled_window), list_view);
  if (mode_landscape)
    {
      GtkWidget *sep = gtk_vseparator_new();
      gchar *cfgval = contacts_db_get_config_tag(CONFIG_LIST, "pos");
      hbox1 = gtk_hbox_new(FALSE, gpe_get_boxspacing());
      gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);
      if ((cfgval) && !strcmp(cfgval, "right"))
        {
          gtk_box_pack_end (GTK_BOX (hbox1), scrolled_window, FALSE, TRUE, 0);
          gtk_box_pack_end (GTK_BOX (hbox1), sep, FALSE, TRUE, 0);
          gtk_box_pack_end (GTK_BOX (hbox1), filter_window, FALSE, TRUE, 0);
        }
      else
        {
          gtk_box_pack_start (GTK_BOX (hbox1), filter_window, FALSE, TRUE, 0);
          gtk_box_pack_start (GTK_BOX (hbox1), sep, FALSE, TRUE, 0);
          gtk_box_pack_start (GTK_BOX (hbox1), scrolled_window, FALSE, TRUE, 0);
        }
      if (cfgval)
        g_free(cfgval);
      g_object_set_data(G_OBJECT(main_window), "hbox", hbox1);
      g_object_set_data(G_OBJECT(main_window), "swlist", scrolled_window);
    }
  else  
    {
      gtk_box_pack_start (GTK_BOX (vbox1), scrolled_window, TRUE, TRUE, 0);
    }    
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Contact"), renderer,
						     "text", 0, NULL);
  if (mode_landscape)
    {
      /* min width = 25% */
      gtk_tree_view_column_set_min_width(column, size_x / 4);
      /* max width = 50% */
      gtk_tree_view_column_set_max_width(column, size_x / 2); 
    }
  gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);
	

  /* connect actions to widgets */
#if MAEMO_VERSION_MAJOR < 5
  g_signal_connect (G_OBJECT (categories_smenu), "changed", 
                    G_CALLBACK (do_search), entry1);
#else
  g_signal_connect (G_OBJECT (categories_picker_button), "value-changed", 
                    G_CALLBACK (do_search), entry1);
#endif
  g_signal_connect (G_OBJECT (entry1), "activate", G_CALLBACK (do_search), entry1);
  g_signal_connect (G_OBJECT (entry1), "changed", G_CALLBACK (schedule_search), NULL);

  g_signal_connect (G_OBJECT (list_view), "button_press_event", 
                    G_CALLBACK (list_button_press_event), list_store);

  g_signal_connect (G_OBJECT (list_view), "button_release_event", 
                    G_CALLBACK (list_button_release_event), list_store);
  
  if (mode_large_screen)
    g_signal_connect (G_OBJECT (list_view), "cursor-changed", 
                      G_CALLBACK (list_view_cursor_changed), lStatus);

  g_signal_connect (G_OBJECT (main_window), "key_press_event", 
                    G_CALLBACK (window_key_press_event), list_view);

  g_signal_connect (G_OBJECT (toolbar), "key_press_event", 
                    G_CALLBACK (toolbar_key_press_event), NULL);
  g_signal_connect (G_OBJECT (entry1), "key_press_event", 
  G_CALLBACK (search_entry_key_press_event), btnNew);
 
  search_entry = entry1;

  /* contact detail panel */
  pDetail = gtk_alignment_new(0, 0, 1, 1);
  
  if (mode_landscape)
    {
      GtkWidget *sep = gtk_vseparator_new();
      gtk_container_set_border_width(GTK_CONTAINER(pDetail), 0);
      gtk_box_pack_start (GTK_BOX (hbox1), sep, FALSE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (hbox1), pDetail, TRUE, TRUE, 0);
    }
  else
    {
      gtk_box_pack_start (GTK_BOX (vbox1), pDetail, FALSE, TRUE, 0);
    }
  tabDetail = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings(GTK_TABLE(tabDetail), gpe_get_boxspacing());
  gtk_table_set_row_spacings(GTK_TABLE(tabDetail), gpe_get_boxspacing());
  gtk_container_set_border_width(GTK_CONTAINER(tabDetail), gpe_get_border() / 2);
  if (mode_large_screen)
    {
      GtkWidget *vport;
      scrolled_window = gtk_scrolled_window_new(NULL, NULL);
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                     GTK_POLICY_AUTOMATIC,
                                     GTK_POLICY_AUTOMATIC);
      vport = gtk_viewport_new(NULL, NULL);
      gtk_viewport_set_shadow_type(GTK_VIEWPORT(vport), GTK_SHADOW_NONE);
      gtk_container_add(GTK_CONTAINER (vport), tabDetail);
      gtk_container_add (GTK_CONTAINER (scrolled_window), vport);
      gtk_container_add (GTK_CONTAINER (pDetail), scrolled_window);
    }
  else
    gtk_container_add (GTK_CONTAINER (pDetail), tabDetail);
  
  /* store pointers to some widgets */
  g_object_set_data (G_OBJECT (main_window), "tabDetail", tabDetail);
  g_object_set_data (G_OBJECT (main_window), "entry", entry1);
  g_object_set_data (G_OBJECT (main_window), "toolbar", toolbar);

  g_signal_connect (G_OBJECT (tree_sel), "changed",
		    G_CALLBACK (selection_made), main_window);
  g_signal_connect (G_OBJECT (filter_sel), "changed",
		    G_CALLBACK (filter_selected), main_window);
#if MAEMO_VERSION_MAJOR < 5
  g_signal_connect (G_OBJECT (main_window), "focus-in-event",
     G_CALLBACK (on_main_focus), entry1);
#endif
  
  return main_window;
}

static void 
osso_top_callback (const gchar* arguments, gpointer ptr)
{
    GtkWindow *window = ptr;

    gtk_window_present (GTK_WINDOW (window));
}

int
main (int argc, char *argv[])
{
  int arg;
  gboolean edit_structure = TRUE;
  gboolean edit_vcard = FALSE;
  gchar *ifile = NULL;
  gchar *efile = NULL;
  GtkTreePath *path;
  gint size_x, size_y;
  
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

  /* screen layout detection */
  size_x = gdk_screen_width();
  size_y = gdk_screen_height();  
  mode_large_screen = (size_x > 240) && (size_y > 320);
  mode_landscape = (size_x > size_y);
  
  /* check command line args */
  while ((arg = getopt(argc, argv, "hi:e:v")) >= 0)
  {
    /* -h show help */
    if (arg == 'h')
        show_help_and_exit ();
    /* -i imports a file */
    if (arg == 'i')
		ifile = optarg;
    /* -e exports a file */
    if (arg == 'e')
		efile = optarg;
    /* -v edits owner's vcard */
    if (arg == 'v')
      edit_vcard = TRUE;
  }    

  /* initialise data backends */ 
  gpe_pim_categories_init ();

  /* are we called to import a file? */
  if (ifile)
    {
      int ret;
      GError *error = NULL;
      
      ret =  import_one_file(ifile, &error);
      if (ret) 
	{
	  g_print(_("Could not import file %s: %s.\n"), ifile, error->message);
	  g_error_free(error);
	  contacts_db_close();
	  exit (EXIT_FAILURE);
	}
      else
	{
	  g_print(_("File %s imported sucessfully.\n"), ifile);      
	  contacts_db_close();
	  exit (EXIT_SUCCESS);
	}
    }
  
  export_init ();

  /* are we called to export a file? */
  if (efile)
    {
      int ret;
      GError *error = NULL;
      
      ret =  export_one_file(efile, &error);
      if (ret) 
	{
	  g_print(_("Could not export file %s: %s.\n"), efile, error->message);
	  g_error_free(error);
	  exit (EXIT_FAILURE);
	}
      else
	{
	  g_print(_("File %s exported sucessfully.\n"), efile);      
	  exit (EXIT_SUCCESS);
	}
    }
  
 /* we are called to edit a users personal vcard */
  if (edit_vcard)
    {
      struct contacts_person *p;
      const gchar *MY_VCARD = g_strdup_printf("%s/.gpe/user.vcf", 
                                              g_get_home_dir());
  
      if (contacts_db_open (TRUE))
        exit (1);
      load_well_known_tags ();
      load_structure (LARGE_STRUCTURE);
      
      p = contacts_db_get_by_uid (1);
      if (!p)
        p = contacts_new_person();
      edit_person (p, _("My Card"), TRUE);
      gtk_main();
      if (save_to_file(1, MY_VCARD, FALSE))
        gpe_perror_box(_("Saving vcard failed"));
      exit (EXIT_SUCCESS);
    }
    
   if (contacts_db_open (FALSE))
    exit (1);

  load_well_known_tags ();

  list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
  filter_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
   
  mainw = create_main (edit_structure);
  popup_menu = create_popup (mainw);
  update_categories ();
  show_details (NULL);

  gtk_signal_connect (GTK_OBJECT (mainw), "destroy", gtk_main_quit, NULL);

  if (mode_large_screen)
    load_structure (LARGE_STRUCTURE);
  else
    load_structure (DEFAULT_STRUCTURE);

  /* load detail panel fields, create widgets */
  if (!mode_landscape)
    load_panel_config ();

  populate_filter();

  /* fire up osso services */
  context = osso_initialize(SERVICE_NAME, SERVICE_VERSION, TRUE, NULL);
  g_assert(context);
  osso_application_set_top_cb(context, osso_top_callback, (gpointer)mainw);
  
  
  gtk_widget_show_all (mainw);
  gtk_widget_grab_focus (GTK_WIDGET (g_object_get_data (G_OBJECT (mainw), "entry")));

  path = gtk_tree_path_new_first();
  gtk_tree_view_set_cursor(GTK_TREE_VIEW(list_view), path, NULL, FALSE);
  gtk_tree_path_free(path);

  gtk_main ();
  return 0;
}
