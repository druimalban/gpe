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

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/smallbox.h>
#include <gpe/errorbox.h>
#include <gpe/gtkdatecombo.h>
#include <gpe/question.h>
#include <gpe/gtksimplemenu.h>
#include <gpe/picturebutton.h>

#include "support.h"
#include "db.h"
#include "structure.h"
#include "proto.h"
#include "callbacks.h"

extern GtkWidget *mainw;
extern gboolean panel_config_has_changed;

static gint panel_active_row = -1;

void
on_bDetAdd_clicked (GtkButton * button, gpointer user_data)
{
  GtkWidget *clist;
  GtkWidget *entry;
  GtkWidget *cbox;
  gchar *strvec[2];

  clist = lookup_widget (mainw, "clist8");
  entry = lookup_widget (mainw, "entry2");
  cbox = lookup_widget (mainw, "cbField");
  strvec[1] = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

  if (strlen (strvec[1]) > 0)
    {
      strvec[0] =
	gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO (cbox)->entry), 0,
				-1);
      gtk_clist_append (GTK_CLIST (clist), strvec);
      gtk_entry_set_text (GTK_ENTRY (entry), "");
      db_add_config_values (CONFIG_PANEL, strvec[1], strvec[0]);
      g_free (strvec[0]);
      g_free (strvec[1]);
      panel_config_has_changed = TRUE;
    }
}


void
on_bDetRemove_clicked (GtkButton * button, gpointer user_data)
{
  GtkWidget *clist;
  gchar *identifier;
  if (panel_active_row > 0)
    {
      clist = lookup_widget (mainw, "clist8");
      gtk_clist_get_text (GTK_CLIST (clist), panel_active_row, 1,
			  &identifier);
      db_delete_config_values (CONFIG_PANEL, identifier);
      gtk_clist_remove (GTK_CLIST (clist), panel_active_row);
      panel_config_has_changed = TRUE;
    }
}


void
on_clist8_select_row (GtkCList * clist,
		      gint row,
		      gint column, GdkEvent * event, gpointer user_data)
{
  panel_active_row = row;
}


void
on_clist8_unselect_row (GtkCList * clist,
			gint row,
			gint column, GdkEvent * event, gpointer user_data)
{
  panel_active_row = -1;
}

void
on_setup_destroy (GtkObject * object, gpointer user_data)
{
  if (panel_config_has_changed)
    load_panel_config ();
}

GtkWidget *
create_pageSetup ()
{
  GtkWidget *wSetup;
  GtkWidget *vbox9;
  GtkWidget *frame2;
  GtkWidget *vbox10;
  GtkWidget *hbox4;
  GtkWidget *cbField;
  GList *cbField_items = NULL;
  GtkWidget *combo_entry1;
  GtkWidget *entry2;
  GtkWidget *bDetAdd;
  GtkWidget *bDetRemove;
  GtkWidget *scrolledwindow1;
  GtkWidget *clist8;
  GtkWidget *label86;
  GtkWidget *label87;
  GtkWidget *frame3;

  gint tagcount, i;
  gchar **taglist;
  gchar *strvec[2];

  wSetup = mainw;

  /*
     glade generated stuff starts here
     this is the container, child in tab page     
   */
  vbox9 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox9, "vbox9");
  gtk_widget_ref (vbox9);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "vbox9", vbox9,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox9);

  frame2 = gtk_frame_new (_("Detail panel"));
  gtk_widget_set_name (frame2, "frame2");
  gtk_widget_ref (frame2);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "frame2", frame2,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame2);
  gtk_box_pack_start (GTK_BOX (vbox9), frame2, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_NONE);

  vbox10 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox10, "vbox10");
  gtk_widget_ref (vbox10);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "vbox10", vbox10,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox10);
  gtk_container_add (GTK_CONTAINER (frame2), vbox10);

  hbox4 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox4, "hbox4");
  gtk_widget_ref (hbox4);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "hbox4", hbox4,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox4);
  gtk_box_pack_start (GTK_BOX (vbox10), hbox4, FALSE, TRUE, 0);

  cbField = gtk_combo_new ();
  gtk_widget_set_name (cbField, "cbField");
  gtk_widget_ref (cbField);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "cbField", cbField,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (cbField);
  gtk_box_pack_start (GTK_BOX (hbox4), cbField, TRUE, TRUE, 0);

  combo_entry1 = GTK_COMBO (cbField)->entry;
  gtk_widget_set_name (combo_entry1, "combo_entry1");
  gtk_widget_ref (combo_entry1);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "combo_entry1", combo_entry1,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (combo_entry1);
  gtk_entry_set_text (GTK_ENTRY (combo_entry1), _("NAME"));

  entry2 = gtk_entry_new ();
  gtk_widget_set_name (entry2, "entry2");
  gtk_widget_ref (entry2);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "entry2", entry2,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (entry2);
  gtk_box_pack_start (GTK_BOX (hbox4), entry2, TRUE, TRUE, 0);

  bDetAdd = gtk_button_new_with_label (_("Add"));
  gtk_widget_set_name (bDetAdd, "bDetAdd");
  gtk_widget_ref (bDetAdd);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "bDetAdd", bDetAdd,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (bDetAdd);
  gtk_box_pack_start (GTK_BOX (hbox4), bDetAdd, FALSE, FALSE, 0);

  bDetRemove = gtk_button_new_with_label (_("Remove"));
  gtk_widget_set_name (bDetRemove, "bDetRemove");
  gtk_widget_ref (bDetRemove);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "bDetRemove", bDetRemove,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (bDetRemove);
  gtk_box_pack_start (GTK_BOX (hbox4), bDetRemove, FALSE, FALSE, 0);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow1, "scrolledwindow1");
  gtk_widget_ref (scrolledwindow1);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "scrolledwindow1",
			    scrolledwindow1,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (vbox10), scrolledwindow1, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  clist8 = gtk_clist_new (2);
  gtk_widget_set_name (clist8, "clist8");
  gtk_widget_ref (clist8);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "clist8", clist8,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (clist8);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), clist8);
  gtk_clist_set_column_width (GTK_CLIST (clist8), 0, 126);
  gtk_clist_set_column_width (GTK_CLIST (clist8), 1, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist8));

  label86 = gtk_label_new (_("Data Field"));
  gtk_widget_set_name (label86, "label86");
  gtk_widget_ref (label86);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "label86", label86,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label86);
  gtk_clist_set_column_widget (GTK_CLIST (clist8), 0, label86);

  label87 = gtk_label_new (_("Label"));
  gtk_widget_set_name (label87, "label87");
  gtk_widget_ref (label87);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "label87", label87,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label87);
  gtk_clist_set_column_widget (GTK_CLIST (clist8), 1, label87);

  frame3 = gtk_frame_new (_("Pages"));
  gtk_widget_set_name (frame3, "frame3");
  gtk_widget_ref (frame3);
  gtk_object_set_data_full (GTK_OBJECT (wSetup), "frame3", frame3,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame3);
  gtk_box_pack_start (GTK_BOX (vbox9), frame3, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame3), GTK_SHADOW_NONE);

  gtk_signal_connect (GTK_OBJECT (bDetAdd), "clicked",
		      GTK_SIGNAL_FUNC (on_bDetAdd_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (bDetRemove), "clicked",
		      GTK_SIGNAL_FUNC (on_bDetRemove_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (clist8), "select_row",
		      GTK_SIGNAL_FUNC (on_clist8_select_row), NULL);
  gtk_signal_connect (GTK_OBJECT (clist8), "unselect_row",
		      GTK_SIGNAL_FUNC (on_clist8_unselect_row), NULL);


  /* handcoded stuff */

  tagcount = db_get_tag_list (&taglist);
  for (i = 0; i < tagcount; i++)
    {
      cbField_items =
	g_list_append (cbField_items, (gpointer) taglist[i + 1]);
    }
  gtk_combo_set_popdown_strings (GTK_COMBO (cbField), cbField_items);
  g_list_free (cbField_items);
  db_free_result (taglist);

  // reused some variables here...
  tagcount = db_get_config_values (CONFIG_PANEL, &taglist);
  for (i = 0; i < tagcount; i++)
    {
      strvec[1] = (taglist[2 * i + 2]);
      strvec[0] = (taglist[2 * i + 3]);
      gtk_clist_append (GTK_CLIST (clist8), strvec);
    }
  db_free_result (taglist);
  return vbox9;
}

void
configure (GtkWidget * widget, gpointer d)
{
  GtkWidget *window = gtk_dialog_new ();
  GtkWidget *notebook = gtk_notebook_new ();
  GtkWidget *editlabel = gtk_label_new (_("Editing Layout"));
  GtkWidget *editbox = edit_structure ();
  GtkWidget *configlabel = gtk_label_new (_("Setup"));
  GtkWidget *configbox = create_pageSetup ();
  GtkWidget *ok_button = gtk_button_new_from_stock (GTK_STOCK_OK);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), editbox, editlabel);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), configbox, configlabel);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->vbox), notebook);
  
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), ok_button, FALSE, FALSE, 0);

  gtk_window_set_default_size (GTK_WINDOW (window), 240, 300);

  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (on_setup_destroy), NULL);

  g_signal_connect_swapped (G_OBJECT (ok_button), "clicked",
			    G_CALLBACK (gtk_widget_destroy), window);

  gtk_window_set_title (GTK_WINDOW (window), _("Contacts: Configuration"));
  gpe_set_window_icon (window, "icon");

  displaymigration_mark_window (window);

  gtk_widget_show_all (window);
}

