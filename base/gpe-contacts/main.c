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

#include "init.h"

#include "interface.h"
#include "support.h"
#include "db.h"
#include "pixmaps.h"
#include "structure.h"
#include "smallbox.h"
#include "errorbox.h"
#include "render.h"

#define MY_PIXMAPS_DIR PREFIX "/share/gpe-contacts/pixmaps"

static GtkWidget *combo;
static GtkWidget *clist;

struct gpe_icon my_icons[] = {
  { "delete" },
  { "new" },
  { "save" },
  { "cancel" },
  { "properties" },
  { "frame", MY_PIXMAPS_DIR "/frame.xpm" },
  { "notebook", MY_PIXMAPS_DIR "/notebook.xpm" },
  { "entry", MY_PIXMAPS_DIR "/entry.xpm" },
  { NULL, NULL }
};

static void
update_combo_categories (void)
{
  GSList *categories = db_get_categories (), *iter;
  GList *list = NULL;
  
  list = g_list_append (list, _("*all*"));

  for (iter = categories; iter; iter = iter->next)
    {
      struct category *c = iter->data;
      list = g_list_append (list, (gpointer)c->name);
    }

  gtk_combo_set_popdown_strings (GTK_COMBO (combo), list);
  g_list_free (list);

  for (iter = categories; iter; iter = iter->next)
    {
      struct category *c = iter->data;
      g_free (c->name);
      g_free (c);
    }

  g_slist_free (categories);
}

static void
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
	    gtk_editable_insert_text (GTK_EDITABLE (w), v->value, strlen (v->value), &pos);
	}
      gtk_object_set_data (GTK_OBJECT (w), "person", p);
    }
  gtk_widget_show (w);
  gtk_widget_grab_focus (entry);
}

static void
new_contact(GtkWidget *widget, gpointer d)
{
  edit_person (NULL);
}

static void
delete_contact(GtkWidget *widget, gpointer d)
{
  if (GTK_CLIST (clist)->selection)
    {
      guint row = (guint)(GTK_CLIST (clist)->selection->data);
      guint uid = (guint)gtk_clist_get_row_data (GTK_CLIST (clist), row);
      if (db_delete_by_uid (uid))
	update_display ();
    }
}

static void
new_category (GtkWidget *w, gpointer p)
{
  gchar *name = smallbox(_("New category"), _("Name"), "");
  if (name && name[0])
    {
      gchar *line_info[1];
      guint id;
      line_info[0] = name;
      if (db_insert_category (name, &id))
	gtk_clist_append (GTK_CLIST (p), line_info);
    }

  update_combo_categories ();
}

static void
delete_category (GtkWidget *w, gpointer p)
{
  update_combo_categories ();
}

static GtkWidget *
config_categories_box(void)
{
  GtkWidget *box = gtk_vbox_new (FALSE, 0);
  GtkWidget *clist = gtk_clist_new (1);
  GtkWidget *toolbar;
  GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget *pw;
  GSList *categories;

  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_SPACE_LINE);

  gtk_widget_show (toolbar);

  pw = gpe_render_icon (NULL, gpe_find_icon ("new"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("New"), 
			   _("New"), _("New"), pw, new_category, clist);

  pw = gpe_render_icon (NULL, gpe_find_icon ("delete"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Delete"), 
			   _("Delete"), _("Delete"), pw, 
			   delete_category, clist);

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
	  gtk_clist_set_row_data (GTK_CLIST (clist), row, 
				  (gpointer) c->id);
	  g_free (c->name);
	  g_free (c);
	  row ++;
	}

      g_slist_free (categories);
    }

  gtk_container_add (GTK_CONTAINER (scrolled), clist);
  gtk_widget_show (clist);
  gtk_widget_show (scrolled);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_NEVER,
				  GTK_POLICY_AUTOMATIC);

  gtk_box_pack_start (GTK_BOX (box), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), scrolled, TRUE, TRUE, 0);

  gtk_widget_show (box);
  return box;
}

static void
configure(GtkWidget *widget, gpointer d)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *notebook = gtk_notebook_new ();
  GtkWidget *editlabel = gtk_label_new (_("Editing layout"));
  GtkWidget *editbox = edit_structure ();
  GtkWidget *categorieslabel = gtk_label_new (_("Categories"));
  GtkWidget *categoriesbox = config_categories_box ();

  gtk_widget_show (notebook);
  gtk_widget_show (editlabel);
  gtk_widget_show (editbox);
  gtk_widget_show (categorieslabel);
  gtk_widget_show (categoriesbox);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    editbox, editlabel);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    categoriesbox, categorieslabel);

  gtk_container_add (GTK_CONTAINER (window), notebook);

  gtk_widget_set_usize (window, 240, 300);

  gtk_widget_show (window);
}

static void 
selection_made( GtkWidget      *clist,
		gint            row,
		gint            column,
		GdkEventButton *event,
		GtkWidget      *widget)
{
  guint id;
    
  if (event->type == GDK_2BUTTON_PRESS)
    {
      struct person *p;
      id = (guint)gtk_clist_get_row_data (GTK_CLIST (clist), row);
      p = db_get_by_uid (id);
      edit_person (p);
    }
}

void
update_display (void)
{
  GSList *items = db_get_entries (), *iter;
  gtk_clist_clear (GTK_CLIST (clist));
  for (iter = items; iter; iter = iter->next)
    {
      struct person *p = iter->data;
      gchar *text[2];
      int row;
      text[0] = p->name;
      text[1] = NULL;
      row = gtk_clist_append (GTK_CLIST (clist), text);
      gtk_clist_set_row_data (GTK_CLIST (clist), row, (gpointer)p->id);
      discard_person (p);
    }
  g_slist_free (items);
}

int
main (int argc, char *argv[])
{
  GtkWidget *mainw;
  GtkWidget *toolbar;
  GtkWidget *pw;

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

  mainw = create_main ();
  combo = lookup_widget (GTK_WIDGET (mainw), "combo1");
  update_combo_categories ();
  clist = lookup_widget (GTK_WIDGET (mainw), "clist1");
  update_display ();
  gtk_widget_show (mainw);
  gtk_signal_connect (GTK_OBJECT (clist), "select_row",
                       GTK_SIGNAL_FUNC (selection_made),
                       NULL);

  gtk_signal_connect (GTK_OBJECT (mainw), "destroy",
		      gtk_main_quit, NULL);

  toolbar = lookup_widget (mainw, "toolbar1");
  pw = gpe_render_icon (mainw->style, gpe_find_icon ("new"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("New Contact"), 
			   _("New Contact"), _("New Contact"),
			   pw, new_contact, NULL);

  pw = gpe_render_icon (mainw->style, gpe_find_icon ("delete"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Delete Contact"), 
			   _("Delete Contact"), _("Delete Contact"), 
			   pw, delete_contact, NULL);

  pw = gpe_render_icon (mainw->style, gpe_find_icon ("properties"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Configure"), 
			   _("Configure"), _("Configure"),
			   pw, configure, NULL);

  load_structure ();

  gtk_main ();
  return 0;
}
