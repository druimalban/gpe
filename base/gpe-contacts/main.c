/*
 * $Id$
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "init.h"

#include "interface.h"
#include "support.h"
#include "db.h"
#include "pixmaps.h"
#include "structure.h"

#define PIXMAPS_DIR "/usr/share/gpe/pixmaps"
#define MY_PIXMAPS_DIR "/usr/share/gpe-contacts/pixmaps"

struct pix my_pix[] = {
  { "close", PIXMAPS_DIR "/close.xpm" },
  //  { "delete", PIXMAPS_DIR "/delete.xpm" },
  { "new", PIXMAPS_DIR "/new.xpm" },
  { "config", PIXMAPS_DIR "/config.xpm" },
  { "frame", MY_PIXMAPS_DIR "/frame.xpm" },
  { "notebook", MY_PIXMAPS_DIR "/notebook.xpm" },
  { "entry", MY_PIXMAPS_DIR "/entry.xpm" },
  { NULL, NULL }
};

static void
new_contact(GtkWidget *widget, gpointer d)
{
  GtkWidget *w = edit_window ();
  GtkWidget *name = lookup_widget (w, "name_entry");
  gtk_widget_show (w);
  gtk_widget_grab_focus (name);
}

static void
configure(GtkWidget *widget, gpointer d)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *notebook = gtk_notebook_new ();
  GtkWidget *editlabel = gtk_label_new ("Edit layout");
  GtkWidget *editbox = edit_structure ();
  GtkWidget *categorieslabel = gtk_label_new ("Categories");
  GtkWidget *categoriesbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *attributeslabel = gtk_label_new ("Attributes");
  GtkWidget *attributesbox = gtk_vbox_new (FALSE, 0);

  gtk_widget_show (notebook);
  gtk_widget_show (editlabel);
  gtk_widget_show (editbox);
  gtk_widget_show (categorieslabel);
  gtk_widget_show (categoriesbox);
  gtk_widget_show (attributeslabel);
  gtk_widget_show (attributesbox);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    attributesbox, attributeslabel);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    editbox, editlabel);
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

  if (load_pixmaps (&my_pix) == FALSE)
    exit (1);

  if (db_open ())
    exit (1);

  mainw = create_main ();
  gtk_widget_show (mainw);

  toolbar = lookup_widget (mainw, "toolbar1");
  p = find_pixmap ("new");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), "New Contact", 
			   "New Contact", "New Contact", 
			   pw, new_contact, NULL);

  p = find_pixmap ("config");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), "Configure", 
			   "Configure", "Configure",
			   pw, configure, NULL);

  load_structure ();

  gtk_main ();
  return 0;
}
