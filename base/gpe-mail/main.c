/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <libintl.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>

#include <gtk/gtk.h>

#include "errorbox.h"

#define _(x) gettext(x)

static guint window_x = 240, window_y = 300;

static GtkWidget *clist_messages;
static GtkWidget *ctree_folders;

static GtkWidget *
config_ident_box (void)
{
  gchar *titles[2];
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *clist;
  GtkWidget *buttons = gtk_hbox_new (FALSE, 0);
  GtkWidget *buttonadd = gtk_button_new_with_label (_("New"));
  GtkWidget *buttonedit = gtk_button_new_with_label (_("Edit"));
  GtkWidget *buttondel = gtk_button_new_with_label (_("Delete"));
  GtkWidget *buttonclose = gtk_button_new_with_label (_("Done"));

  titles[0] = _("Name");
  titles[1] = _("Address");

  clist = gtk_clist_new_with_titles (2, titles);

  gtk_box_pack_start (GTK_BOX (buttons), buttonadd, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons), buttondel, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons), buttonedit, TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (buttons), buttonclose, TRUE, TRUE, 0);
  
  gtk_box_pack_start (GTK_BOX (vbox), clist, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), buttons, FALSE, FALSE, 0);

  return vbox;
}

static GtkWidget *
config_servers_box (void)
{
  gchar *titles[2];
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *clist;
  GtkWidget *buttons = gtk_hbox_new (FALSE, 0);
  GtkWidget *buttonadd = gtk_button_new_with_label (_("New"));
  GtkWidget *buttonedit = gtk_button_new_with_label (_("Edit"));
  GtkWidget *buttondel = gtk_button_new_with_label (_("Delete"));
  GtkWidget *buttonclose = gtk_button_new_with_label (_("Done"));

  titles[0] = _("Server");
  titles[1] = _("Login");

  clist = gtk_clist_new_with_titles (2, titles);

  gtk_box_pack_start (GTK_BOX (buttons), buttonadd, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons), buttondel, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons), buttonedit, TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (buttons), buttonclose, TRUE, TRUE, 0);
  
  gtk_box_pack_start (GTK_BOX (vbox), clist, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), buttons, FALSE, FALSE, 0);

  return vbox;
}

static void
configure (void)
{
  GtkWidget *cfgwin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *notebook = gtk_notebook_new ();
  GtkWidget *labelident = gtk_label_new (_("Identities"));
  GtkWidget *labelserver = gtk_label_new (_("Servers"));
  GtkWidget *vboxident = config_ident_box ();
  GtkWidget *vboxserver = config_servers_box ();

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vboxident, labelident);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vboxserver, labelserver);

  gtk_widget_set_usize (cfgwin, window_x, window_y);

  gtk_container_add (GTK_CONTAINER (cfgwin), notebook);

  gtk_widget_show_all (cfgwin);
}

static void
do_write (void)
{
  GtkWidget *writewin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *text = gtk_text_new (NULL, NULL);
  GtkWidget *optionfrom = gtk_option_menu_new ();
  GtkWidget *comboto = gtk_combo_new ();
  GtkWidget *entrysubject = gtk_entry_new ();
  GtkWidget *labelfrom = gtk_label_new (_("From:"));
  GtkWidget *labelto = gtk_label_new (_("To:"));
  GtkWidget *labelsubject = gtk_label_new (_("Subject:"));
  GtkWidget *table = gtk_table_new (3, 2, FALSE);
  GtkWidget *hboxfrom = gtk_hbox_new (FALSE, 0);
  GtkWidget *hboxto = gtk_hbox_new (FALSE, 0);
  GtkWidget *hboxsubject = gtk_hbox_new (FALSE, 0);
  GtkWidget *buttonsend = gtk_button_new_with_label (_("Send"));
  GtkWidget *buttoncancel = gtk_button_new_with_label (_("Cancel"));
  GtkWidget *hboxbutton = gtk_hbox_new (FALSE, 0);

  gtk_box_pack_start (GTK_BOX (hboxfrom), labelfrom, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hboxto), labelto, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hboxsubject), labelsubject, FALSE, FALSE, 0);

  gtk_table_attach_defaults (GTK_TABLE (table), hboxfrom,     0, 1, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (table), hboxto,       0, 1, 1, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), hboxsubject,  0, 1, 2, 3);
  gtk_table_attach_defaults (GTK_TABLE (table), optionfrom,   1, 2, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (table), comboto,      1, 2, 1, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), entrysubject, 1, 2, 2, 3);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);

  gtk_box_pack_start (GTK_BOX (hboxbutton), buttoncancel, TRUE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (hboxbutton), buttonsend, TRUE, TRUE, 2);

  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), text, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hboxbutton, FALSE, FALSE, 2);

  gtk_container_add (GTK_CONTAINER (writewin), vbox);

  gtk_widget_show_all (writewin);

  gtk_widget_grab_focus (GTK_COMBO (comboto)->entry);
}

static GtkItemFactoryEntry menu_items[] = {
  { "/File",              NULL,         NULL, 0, "<Branch>" },
  { "/File/Configure",    NULL,         configure, 0, NULL },
  { "/File/Quit",         NULL,         gtk_main_quit, 0, NULL },
  { "/Mail",              NULL,         NULL, 0, "<Branch>" },
  { "/Mail/Write",        NULL,         do_write, 0, NULL },
  { "/Help"       ,       NULL,         NULL, 0, "<LastBranch>" },
  { "/Help/About",        NULL,         NULL, 0, NULL },
};

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkItemFactory *item_factory;
  GtkWidget *menubar, *vbox;
  GtkWidget *viewport1, *viewport2;
  GtkWidget *paned;
  gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);
  gchar *titles[2];

  gtk_set_locale ();
  gtk_init (&argc, &argv);

  window  = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", 
                                       NULL);
  gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);
  menubar = gtk_item_factory_get_widget (item_factory, "<main>");

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  vbox = gtk_vbox_new (FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);

  viewport1 = gtk_scrolled_window_new (NULL, NULL);
  viewport2 = gtk_scrolled_window_new (NULL, NULL);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (viewport1), 
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (viewport2), 
				  GTK_POLICY_NEVER, 
				  GTK_POLICY_ALWAYS);

  titles[0] = NULL;
  titles[1] = _("From");
  titles[2] = _("Subject");

  ctree_folders = gtk_ctree_new (3, 1);
  gtk_container_add (GTK_CONTAINER (viewport1), ctree_folders);

  clist_messages = gtk_clist_new_with_titles (3, titles);
  gtk_container_add (GTK_CONTAINER (viewport2), clist_messages);

  paned = gtk_vpaned_new ();

  gtk_paned_pack1 (GTK_PANED (paned), viewport1, TRUE, TRUE);
  gtk_paned_pack2 (GTK_PANED (paned), viewport2, TRUE, TRUE);

  gtk_box_pack_start (GTK_BOX (vbox), paned, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (window), vbox);

  gtk_widget_set_usize (window, window_x, window_y);

  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
