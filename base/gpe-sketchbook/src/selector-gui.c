#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "selector-gui.h"
#include "selector-cb.h"

#include "_support.h"


GtkWidget*
create_window_selector (void)
{
  GtkWidget *window_selector;
  GtkWidget *vbox1;
  GtkWidget *hbox_selector_toolbar;
  GtkWidget *button_selector_new;
  GtkWidget *pixmap18;
  GtkWidget *button_selector_open;
  GtkWidget *pixmap16;
  GtkWidget *button_selector_delete;
  GtkWidget *pixmap21;
  GtkWidget *button_sketchpad_view;
  GtkWidget *pixmap20;
  GtkWidget *button_selector_about;
  GtkWidget *pixmap19;
  GtkWidget *scrolledwindow_selector_clist;
  GtkWidget *clist_selector;
  GtkWidget *label_Clist_column1;

  window_selector = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (window_selector), "window_selector", window_selector);
  gtk_window_set_default_size (GTK_WINDOW (window_selector), 240, 280);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox1);
  gtk_object_set_data_full (GTK_OBJECT (window_selector), "vbox1", vbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (window_selector), vbox1);

  hbox_selector_toolbar = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox_selector_toolbar);
  gtk_object_set_data_full (GTK_OBJECT (window_selector), "hbox_selector_toolbar", hbox_selector_toolbar,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox_selector_toolbar);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox_selector_toolbar, FALSE, FALSE, 0);

  button_selector_new = gtk_button_new ();
  gtk_widget_ref (button_selector_new);
  gtk_object_set_data_full (GTK_OBJECT (window_selector), "button_selector_new", button_selector_new,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_selector_new);
  gtk_box_pack_start (GTK_BOX (hbox_selector_toolbar), button_selector_new, FALSE, FALSE, 0);
  gtk_button_set_relief (GTK_BUTTON (button_selector_new), GTK_RELIEF_NONE);

  pixmap18 = create_pixmap (window_selector, "gnew.xpm");
  gtk_widget_ref (pixmap18);
  gtk_object_set_data_full (GTK_OBJECT (window_selector), "pixmap18", pixmap18,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap18);
  gtk_container_add (GTK_CONTAINER (button_selector_new), pixmap18);

  button_selector_open = gtk_button_new ();
  gtk_widget_ref (button_selector_open);
  gtk_object_set_data_full (GTK_OBJECT (window_selector), "button_selector_open", button_selector_open,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_selector_open);
  gtk_box_pack_start (GTK_BOX (hbox_selector_toolbar), button_selector_open, FALSE, FALSE, 0);
  gtk_button_set_relief (GTK_BUTTON (button_selector_open), GTK_RELIEF_NONE);

  pixmap16 = create_pixmap (window_selector, "gopen.xpm");
  gtk_widget_ref (pixmap16);
  gtk_object_set_data_full (GTK_OBJECT (window_selector), "pixmap16", pixmap16,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap16);
  gtk_container_add (GTK_CONTAINER (button_selector_open), pixmap16);

  button_selector_delete = gtk_button_new ();
  gtk_widget_ref (button_selector_delete);
  gtk_object_set_data_full (GTK_OBJECT (window_selector), "button_selector_delete", button_selector_delete,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_selector_delete);
  gtk_box_pack_start (GTK_BOX (hbox_selector_toolbar), button_selector_delete, FALSE, FALSE, 0);
  gtk_button_set_relief (GTK_BUTTON (button_selector_delete), GTK_RELIEF_NONE);

  pixmap21 = create_pixmap (window_selector, "gdelete.xpm");
  gtk_widget_ref (pixmap21);
  gtk_object_set_data_full (GTK_OBJECT (window_selector), "pixmap21", pixmap21,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap21);
  gtk_container_add (GTK_CONTAINER (button_selector_delete), pixmap21);

  button_sketchpad_view = gtk_button_new ();
  gtk_widget_ref (button_sketchpad_view);
  gtk_object_set_data_full (GTK_OBJECT (window_selector), "button_sketchpad_view", button_sketchpad_view,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_sketchpad_view);
  gtk_box_pack_start (GTK_BOX (hbox_selector_toolbar), button_sketchpad_view, FALSE, FALSE, 0);
  gtk_button_set_relief (GTK_BUTTON (button_sketchpad_view), GTK_RELIEF_NONE);

  pixmap20 = create_pixmap (window_selector, "gsketchpad.xpm");
  gtk_widget_ref (pixmap20);
  gtk_object_set_data_full (GTK_OBJECT (window_selector), "pixmap20", pixmap20,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap20);
  gtk_container_add (GTK_CONTAINER (button_sketchpad_view), pixmap20);

  button_selector_about = gtk_button_new ();
  gtk_widget_ref (button_selector_about);
  gtk_object_set_data_full (GTK_OBJECT (window_selector), "button_selector_about", button_selector_about,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_selector_about);
  gtk_box_pack_end (GTK_BOX (hbox_selector_toolbar), button_selector_about, FALSE, FALSE, 0);
  gtk_button_set_relief (GTK_BUTTON (button_selector_about), GTK_RELIEF_NONE);

  pixmap19 = create_pixmap (window_selector, "gabout.xpm");
  gtk_widget_ref (pixmap19);
  gtk_object_set_data_full (GTK_OBJECT (window_selector), "pixmap19", pixmap19,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap19);
  gtk_container_add (GTK_CONTAINER (button_selector_about), pixmap19);

  scrolledwindow_selector_clist = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow_selector_clist);
  gtk_object_set_data_full (GTK_OBJECT (window_selector), "scrolledwindow_selector_clist", scrolledwindow_selector_clist,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow_selector_clist);
  gtk_box_pack_start (GTK_BOX (vbox1), scrolledwindow_selector_clist, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_selector_clist), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  clist_selector = gtk_clist_new (1);
  gtk_widget_ref (clist_selector);
  gtk_object_set_data_full (GTK_OBJECT (window_selector), "clist_selector", clist_selector,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (clist_selector);
  gtk_container_add (GTK_CONTAINER (scrolledwindow_selector_clist), clist_selector);
  gtk_clist_set_column_width (GTK_CLIST (clist_selector), 0, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist_selector));

  label_Clist_column1 = gtk_label_new ("column 1");
  gtk_widget_ref (label_Clist_column1);
  gtk_object_set_data_full (GTK_OBJECT (window_selector), "label_Clist_column1", label_Clist_column1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label_Clist_column1);
  gtk_clist_set_column_widget (GTK_CLIST (clist_selector), 0, label_Clist_column1);

  gtk_signal_connect (GTK_OBJECT (window_selector), "destroy",
                      GTK_SIGNAL_FUNC (on_window_selector_destroy),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_selector_new), "clicked",
                      GTK_SIGNAL_FUNC (on_button_selector_new_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_selector_open), "clicked",
                      GTK_SIGNAL_FUNC (on_button_selector_open_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_selector_delete), "clicked",
                      GTK_SIGNAL_FUNC (on_button_selector_delete_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_sketchpad_view), "clicked",
                      GTK_SIGNAL_FUNC (on_button_sketchpad_view_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_selector_about), "clicked",
                      GTK_SIGNAL_FUNC (on_button_selector_about_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (clist_selector), "select_row",
                      GTK_SIGNAL_FUNC (on_clist_selector_select_row),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (clist_selector), "click_column",
                      GTK_SIGNAL_FUNC (on_clist_selector_click_column),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (clist_selector), "unselect_row",
                      GTK_SIGNAL_FUNC (on_clist_selector_unselect_row),
                      NULL);

  return window_selector;
}

