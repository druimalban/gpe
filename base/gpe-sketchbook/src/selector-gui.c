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
#include "selector.h"

#include "_support.h" //FIXME: pixmaps, to be removed


GtkWidget * create_window_selector(){

  GtkWidget *window_selector;
  GtkWidget *vbox1;

  //toolbar
  GtkWidget *hbox_selector_toolbar;
  GtkWidget *button_selector_new;    GtkWidget *pixmap18;
  GtkWidget *button_selector_open;   GtkWidget *pixmap16;
  GtkWidget *button_selector_delete; GtkWidget *pixmap21;
  GtkWidget *button_sketchpad_view;  GtkWidget *pixmap20;
  GtkWidget *button_selector_about;  GtkWidget *pixmap19;

  //clist
  GtkWidget *scrolledwindow_selector_clist;
  GtkWidget *clist_selector;
  GtkWidget *label_Clist_column1;

  //--Main window
  window_selector = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window_selector), 240, 280);
  gtk_signal_connect (GTK_OBJECT (window_selector), "destroy",
                      GTK_SIGNAL_FUNC (on_window_selector_destroy),
                      NULL);

  //--Clist
  clist_selector = gtk_clist_new (1);
  set_selector_clist(GTK_CLIST(clist_selector));//set a ref to the clist
  gtk_clist_set_column_width (GTK_CLIST (clist_selector), 0, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist_selector));
  label_Clist_column1 = gtk_label_new ("column 1");
  gtk_clist_set_column_widget (GTK_CLIST (clist_selector), 0, label_Clist_column1);

  scrolledwindow_selector_clist = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_selector_clist),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scrolledwindow_selector_clist), clist_selector);

  gtk_signal_connect (GTK_OBJECT (clist_selector), "select_row",
                      GTK_SIGNAL_FUNC (on_clist_selector_select_row), NULL);
  gtk_signal_connect (GTK_OBJECT (clist_selector), "click_column",
                      GTK_SIGNAL_FUNC (on_clist_selector_click_column), NULL);
  gtk_signal_connect (GTK_OBJECT (clist_selector), "unselect_row",
                      GTK_SIGNAL_FUNC (on_clist_selector_unselect_row), NULL);

  //--Toolbar
  button_selector_new    = gtk_button_new ();
  button_selector_open   = gtk_button_new ();
  button_selector_delete = gtk_button_new ();
  button_sketchpad_view  = gtk_button_new ();
  button_selector_about  = gtk_button_new ();

  //no relief
  gtk_button_set_relief (GTK_BUTTON (button_selector_about),  GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (button_sketchpad_view),  GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (button_selector_delete), GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (button_selector_open),   GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (button_selector_new),    GTK_RELIEF_NONE);

  //FIXME: pixmaps
  pixmap18 = create_pixmap (window_selector, "gnew.xpm");
  gtk_container_add (GTK_CONTAINER (button_selector_new), pixmap18);
  pixmap16 = create_pixmap (window_selector, "gopen.xpm");
  gtk_container_add (GTK_CONTAINER (button_selector_open), pixmap16);
  pixmap21 = create_pixmap (window_selector, "gdelete.xpm");
  gtk_container_add (GTK_CONTAINER (button_selector_delete), pixmap21);
  pixmap20 = create_pixmap (window_selector, "gsketchpad.xpm");
  gtk_container_add (GTK_CONTAINER (button_sketchpad_view), pixmap20);
  pixmap19 = create_pixmap (window_selector, "gabout.xpm");
  gtk_container_add (GTK_CONTAINER (button_selector_about), pixmap19);

  //signals connection
  gtk_signal_connect (GTK_OBJECT (button_selector_new), "clicked",
                      GTK_SIGNAL_FUNC (on_button_selector_new_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (button_selector_open), "clicked",
                      GTK_SIGNAL_FUNC (on_button_selector_open_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (button_selector_delete), "clicked",
                      GTK_SIGNAL_FUNC (on_button_selector_delete_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (button_sketchpad_view), "clicked",
                      GTK_SIGNAL_FUNC (on_button_sketchpad_view_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (button_selector_about), "clicked",
                      GTK_SIGNAL_FUNC (on_button_selector_about_clicked), NULL);

  //--packing
  hbox_selector_toolbar = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_selector_toolbar), button_selector_new,    FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_selector_toolbar), button_selector_open,   FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_selector_toolbar), button_selector_delete, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_selector_toolbar), button_sketchpad_view,  FALSE, FALSE, 0);
  gtk_box_pack_end   (GTK_BOX (hbox_selector_toolbar), button_selector_about,  FALSE, FALSE, 0);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox_selector_toolbar,         FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox1), scrolledwindow_selector_clist, TRUE,  TRUE,  0);

  gtk_container_add (GTK_CONTAINER (window_selector), vbox1);

  //show all except the toplevel window
  gtk_widget_show_all(vbox1);

  return window_selector;
}

