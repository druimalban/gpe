/* GPE Mileage
 * Copyright (C) 2005  Rene Wagner <rw@handhelds.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <gtk/gtk.h>
#include <glade/glade.h>
#include "gui.h"
#include "preferences.h"
#include "units.h"

#include "gpe-mileage-i18n.h"

enum
{
  DESCRIPTION_COLUMN,
  UNIT_COLUMN,
  N_COLUMNS
};

typedef struct preferences_dlg_s
{
  /* GUI elements */
  GtkDialog *dlg_window;
} preferences_dlg_t;

void
preferences_dlg_destroy_user_data (gpointer data)
{
  preferences_dlg_t *preferences_dlg = (preferences_dlg_t *) data;

  /* g_print ("destroy\n"); */
  g_free (preferences_dlg);
}

void
close_clicked (GtkWidget *w, gpointer data)
{
  preferences_dlg_t *preferences_dlg = (preferences_dlg_t *) data;

  /* destroy dialog window */
  gtk_widget_destroy (GTK_WIDGET (preferences_dlg->dlg_window));
}

void
unit_changed (GtkWidget *widget, gpointer data)
{
  GtkComboBox *box = GTK_COMBO_BOX (widget);
  gint unit_type = GPOINTER_TO_INT (data);
  GtkListStore *units_store = GTK_LIST_STORE (gtk_combo_box_get_model (box));
  GtkTreeIter iter;
  gboolean set = FALSE;
  gint selected_unit = -1;

  set = gtk_combo_box_get_active_iter (box, &iter);
  if (set)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (units_store), &iter,
  	                  UNIT_COLUMN, &selected_unit,
			  -1);

      if (selected_unit > -1)
        preferences_set_unit (unit_type, selected_unit);
    }
}

void
populate_units_combo_box (GladeXML *ui, const char *box_id, UnitType type)
{
  GtkComboBox *box = GTK_COMBO_BOX (gui_glade_xml_get_widget (ui, box_id));
  GtkListStore *units_store;
  GtkTreeIter iter;
  gint i;

  units_store = gtk_list_store_new (N_COLUMNS,
                                    G_TYPE_STRING,
				    G_TYPE_INT);

  g_object_set (box,
                "model", units_store,
		NULL);

  for (i = 0; i < units_n_units (type); i++)
    {
      gtk_list_store_append (units_store, &iter);
      gtk_list_store_set (units_store, &iter,
                          DESCRIPTION_COLUMN, units_description (type, i),
			  UNIT_COLUMN, i,
			  -1);

      if (i == preferences_get_unit (type))
        gtk_combo_box_set_active_iter (box, &iter);
    }
}

void
preferences_dlg_open (GtkWindow *parent)
{
  preferences_dlg_t *preferences_dlg = g_new0(preferences_dlg_t, 1);
  GladeXML *ui = gui_glade_xml_new ("preferences_dlg");
  GtkDialog *dlg = GTK_DIALOG (gui_glade_xml_get_widget (ui, "preferences_dlg"));

  gtk_window_set_transient_for (GTK_WINDOW (dlg), parent);

  /* populate preferences_dlg struct */
  preferences_dlg->dlg_window = dlg;

  /* store dialog struct as user data so it can be deleted on window destruction */
  g_object_set_data_full (G_OBJECT (dlg), "user_data", preferences_dlg, preferences_dlg_destroy_user_data);

  populate_units_combo_box (ui, "length_unit_combo_box", UNITS_LENGTH);
  populate_units_combo_box (ui, "capacity_unit_combo_box", UNITS_CAPACITY);
  populate_units_combo_box (ui, "monetary_unit_combo_box", UNITS_MONETARY);
  populate_units_combo_box (ui, "mileage_unit_combo_box", UNITS_MILEAGE);

  glade_xml_signal_connect_data (ui,
                                 "close_clicked",
                                 G_CALLBACK (close_clicked),
                                 preferences_dlg);
  glade_xml_signal_connect_data (ui,
                                 "length_unit_changed",
                                 G_CALLBACK (unit_changed),
                                 GINT_TO_POINTER (UNITS_LENGTH));
  glade_xml_signal_connect_data (ui,
                                 "capacity_unit_changed",
                                 G_CALLBACK (unit_changed),
                                 GINT_TO_POINTER (UNITS_CAPACITY));
  glade_xml_signal_connect_data (ui,
                                 "monetary_unit_changed",
                                 G_CALLBACK (unit_changed),
                                 GINT_TO_POINTER (UNITS_MONETARY));
  glade_xml_signal_connect_data (ui,
                                 "mileage_unit_changed",
                                 G_CALLBACK (unit_changed),
                                 GINT_TO_POINTER (UNITS_MILEAGE));
}
