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

#define PIXMAPS_DIR "/usr/share/gpe/pixmaps"
#define MY_PIXMAPS_DIR "/usr/share/gpe-contacts/pixmaps"

static GtkWidget *combo;

struct pix my_pix[] = {
  { "delete", MY_PIXMAPS_DIR "/trash.png" },
  { "new", MY_PIXMAPS_DIR "/new.png" },
  { "config", MY_PIXMAPS_DIR "/preferences.png" },
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
      struct attribute *c = iter->data;
      list = g_list_append (list, (gpointer)c->name);
    }

  gtk_combo_set_popdown_strings (GTK_COMBO (combo), list);
  g_list_free (list);

  for (iter = categories; iter; iter = iter->next)
    {
      struct attribute *c = iter->data;
      g_free (c->name);
      g_free (c);
    }

  g_slist_free (categories);
}

static void
new_contact(GtkWidget *widget, gpointer d)
{
  GtkWidget *w = edit_window ();
  GtkWidget *entry = lookup_widget (w, "name_entry");
  gtk_widget_show (w);
  gtk_widget_grab_focus (entry);
}

static void
delete_contact(GtkWidget *widget, gpointer d)
{
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

static void
new_attribute (GtkWidget *w, gpointer p)
{
  gchar *name;
  gchar *desc;
  struct box_desc dx[3];

  dx[0].label = _("Tag");
  dx[1].label = _("Description");
  dx[2].label = NULL;
  dx[0].value = NULL;
  dx[1].value = NULL;
  dx[2].value = NULL;

  if (smallbox_x (_("New attribute"), dx) == TRUE)
    {
      name = dx[0].value;
      desc = dx[1].value;

      if (name[0])
	{
	  gchar *line_info[2];
	  guint nr = atoi (name);
	  if (nr == 0)
	    {
	      gpe_error_box (_("Invalid attribute tag"));
	      return;
	    }
	  if (desc[0] == 0)
	    {
	      gpe_error_box (_("No description given"));
	      return;
	    }
	  
	  line_info[0] = name;
	  line_info[1] = desc;
	  if (db_insert_attribute (nr, desc))
	    {
	      gtk_clist_append (GTK_CLIST (p), line_info);
	      gtk_clist_columns_autosize (GTK_CLIST (p));
	    }
	}
      else
	{
	  g_free (name);
	  g_free (desc);
	}
    }
}

static void
delete_attribute (GtkWidget *w, gpointer p)
{
}

static GtkWidget *
config_categories_box(void)
{
  GtkWidget *box = gtk_vbox_new (FALSE, 0);
  GtkWidget *clist = gtk_clist_new (1);
  GtkWidget *toolbar;
  GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);
  struct pix *p;
  GtkWidget *pw;
  GSList *categories;

  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_SPACE_LINE);

  gtk_widget_show (toolbar);

  p = find_pixmap ("new");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("New"), 
			   _("New"), _("New"), pw, new_category, clist);

  p = find_pixmap ("delete");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
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
	  struct attribute *c = iter->data;
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

static GtkWidget *
config_attributes_box(void)
{
  GtkWidget *box = gtk_vbox_new (FALSE, 0);
  GtkWidget *clist = gtk_clist_new (2);
  GtkWidget *toolbar;
  GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);
  struct pix *p;
  GtkWidget *pw;
  GSList *attrs;

  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_SPACE_LINE);

  gtk_widget_show (toolbar);

  p = find_pixmap ("new");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("New"), 
			   _("New"), _("New"), pw, new_attribute, clist);

  p = find_pixmap ("delete");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Delete"), 
			   _("Delete"), _("Delete"), pw, delete_attribute, 
			   clist);
  gtk_container_add (GTK_CONTAINER (scrolled), clist);
  gtk_widget_show (clist);
  gtk_widget_show (scrolled);

  attrs = db_get_attributes ();
  if (attrs)
    {
      GSList *iter;
      guint row = 0;
      
      for (iter = attrs; iter; iter = iter->next)
	{
	  gchar *line_info[2];
	  char buf[10];
	  struct attribute *c = iter->data;
	  sprintf (buf, "%d", c->id);
	  line_info[0] = buf;
	  line_info[1] = c->name;
	  gtk_clist_append (GTK_CLIST (clist), line_info);
	  gtk_clist_set_row_data (GTK_CLIST (clist), row, 
				  (gpointer) c->id);
	  g_free (c->name);
	  g_free (c);
	  row ++;
	}

      g_slist_free (attrs);
    }
  gtk_clist_columns_autosize (GTK_CLIST (clist));

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
  GtkWidget *attributeslabel = gtk_label_new (_("Attributes"));
  GtkWidget *categoriesbox = config_categories_box ();
  GtkWidget *attributesbox = config_attributes_box ();

  gtk_widget_show (notebook);
  gtk_widget_show (editlabel);
  gtk_widget_show (editbox);
  gtk_widget_show (categorieslabel);
  gtk_widget_show (categoriesbox);
  gtk_widget_show (attributeslabel);
  gtk_widget_show (attributesbox);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    editbox, editlabel);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    attributesbox, attributeslabel);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    categoriesbox, categorieslabel);

  gtk_container_add (GTK_CONTAINER (window), notebook);

  gtk_widget_set_usize (window, 240, 300);

  gtk_widget_show (window);
}

int
main (int argc, char *argv[])
{
  GtkWidget *mainw;
  GtkWidget *toolbar;
  struct pix *p;
  GtkWidget *pw;

#ifdef ENABLE_NLS
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
#endif

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (load_pixmaps (my_pix) == FALSE)
    exit (1);

  if (db_open ())
    exit (1);

  mainw = create_main ();
  combo = lookup_widget (GTK_WIDGET (mainw), "combo1");
  update_combo_categories ();
  gtk_widget_show (mainw);

  gtk_signal_connect (GTK_OBJECT (mainw), "destroy",
		      gtk_main_quit, NULL);

  toolbar = lookup_widget (mainw, "toolbar1");
  p = find_pixmap ("new");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("New Contact"), 
			   _("New Contact"), _("New Contact"),
			   pw, new_contact, NULL);

  p = find_pixmap ("delete");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Delete Contact"), 
			   _("Delete Contact"), _("Delete Contact"), 
			   pw, delete_contact, NULL);

  p = find_pixmap ("config");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Configure"), 
			   _("Configure"), _("Configure"),
			   pw, configure, NULL);

  load_structure ();

  gtk_main ();
  return 0;
}
