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
#include "mileage_db.h"
#include "fuel_item.h"
#include "gui.h"
#include "car.h"
#include "view-common.h"
#include "preferences.h"
#include "units.h"

#include "gpe-mileage-i18n.h"

typedef void (*refresh_callback)(gpointer data, gint selected_car_id);

typedef struct new_dlg_s
{
  /* Callback */
  GCallback c_handler;
  gpointer c_user_data;
  /* GUI elements */
  GtkDialog *dlg_window;
  GtkComboBoxEntry *car_entry;
  GtkSpinButton *odometer_spin;
  GtkSpinButton *fuel_spin;
  GtkSpinButton *price_spin;
  GtkEntry *comment_entry;
  GtkButton *ok_button;
  /* backing data structures */
  GtkListStore *cars_list_store;
} new_dlg_t;

void
destroy_user_data (gpointer data)
{
  new_dlg_t *new_dlg = (new_dlg_t *) data;

  /* g_print ("destroy\n"); */
  g_free (new_dlg);
}

void
ok_clicked (GtkWidget *w, gpointer data)
{
  new_dlg_t *new_dlg = (new_dlg_t *) data;
  car_t *new_car = NULL;
  fuel_item_t *new_f = NULL;
  GtkEntry *entry = GTK_ENTRY (GTK_BIN (new_dlg->car_entry)->child);
  gchar *descr = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
  gchar *comment = gtk_editable_get_chars (GTK_EDITABLE (new_dlg->comment_entry), 0, -1);
  gdouble odometer_reading = 0, fuel = 0, price = 0;
  
  g_object_get (new_dlg->odometer_spin, "value", &odometer_reading, NULL);
  g_object_get (new_dlg->fuel_spin, "value", &fuel, NULL);
  g_object_get (new_dlg->price_spin, "value", &price, NULL);

  /* check if the car is in the db already.  */
  new_car = mileage_db_get_car_by_description (descr);
  if (!new_car)
    {
      /* otherwise add it.  */
      new_car = car_new (-1, descr);
      new_car->id = mileage_db_add_car (new_car);
    }

  /* FIXME: get current date! */
  new_f = fuel_item_new (-1,
                         new_car->id,
			 0,
			 odometer_reading,
			 fuel,
			 price,
			 comment);

  mileage_db_add_fuel_item (new_f);

  /* call refresh callback */
  ((refresh_callback) (*new_dlg->c_handler)) (new_dlg->c_user_data, new_car->id);

  /* destroy dialog window */
  gtk_widget_destroy (GTK_WIDGET (new_dlg->dlg_window));
  
  fuel_item_free (new_f);
  car_free (new_car);
  g_free (comment);
  g_free (descr);
}

void
cancel_clicked (GtkWidget *w, gpointer data)
{
  new_dlg_t *new_dlg = (new_dlg_t *) data;

  /* destroy dialog window */
  gtk_widget_destroy (GTK_WIDGET (new_dlg->dlg_window));
}

void
car_entry_changed (GtkWidget *w, gpointer data)
{
  new_dlg_t *new_dlg = (new_dlg_t *) data;
  GtkEntry *entry = GTK_ENTRY (GTK_BIN (new_dlg->car_entry)->child);
  gchar *descr = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

  car_t *car = NULL;
  fuel_item_t *new_f = NULL;

  /* description must be non-empty for the entry to be valid.  */
  if (descr && g_utf8_strlen (descr, -1))
    gtk_widget_set_sensitive (GTK_WIDGET (new_dlg->ok_button), TRUE);
  else
    gtk_widget_set_sensitive (GTK_WIDGET (new_dlg->ok_button), FALSE);

  car = mileage_db_get_car_by_description (descr);
  if (car)
    {
      GSList *f_list = mileage_db_get_fuel_items_by_car_id (car->id);
      if (f_list)
        {
	  gdouble new_min = 0;

	  f_list = g_slist_reverse (f_list);
	  new_min = ((fuel_item_t *)f_list->data)->odometer_reading + 0.01;

          /* FIXME: make this a global constant or something.  */
	  gtk_spin_button_set_range (new_dlg->odometer_spin, new_min, 1000000);
	  fuel_item_slist_free (f_list);
	}
      else
        {
          /* FIXME: make this a global constant or something.  */
          gtk_spin_button_set_range (new_dlg->odometer_spin, 0, 1000000);
	}
      car_free (car);
    }
  else
    {
      /* FIXME: make this a global constant or something.  */
      gtk_spin_button_set_range (new_dlg->odometer_spin, 0, 1000000);
    }
  g_free (descr);
}

void
new_fuel_item_dlg_open (GtkWindow *parent, car_t *active_car, GCallback c_handler, gpointer gobject)
{
  new_dlg_t *new_dlg = g_new0(new_dlg_t, 1);
  GSList *cars_list = NULL;
  GladeXML *ui = gui_glade_xml_new ("new_fuel_item_dlg");
  GtkDialog *dlg = GTK_DIALOG (gui_glade_xml_get_widget (ui, "new_fuel_item_dlg"));
  const gchar* length_unit_short = units_short_description (UNITS_LENGTH, preferences_get_unit (UNITS_LENGTH));
  const gchar* capacity_unit_short = units_short_description (UNITS_CAPACITY, preferences_get_unit (UNITS_CAPACITY));
  const gchar* monetary_unit_short = units_short_description (UNITS_MONETARY, preferences_get_unit (UNITS_MONETARY));
 
  gtk_window_set_transient_for (GTK_WINDOW (dlg), parent);

  /* populate new_dlg struct */
  new_dlg->c_handler = c_handler;
  new_dlg->c_user_data = gobject;
  new_dlg->dlg_window = dlg;
  new_dlg->car_entry = GTK_COMBO_BOX_ENTRY (gui_glade_xml_get_widget (ui, "car_entry"));
  new_dlg->odometer_spin = GTK_SPIN_BUTTON (gui_glade_xml_get_widget (ui, "odometer_spin"));
  new_dlg->fuel_spin = GTK_SPIN_BUTTON (gui_glade_xml_get_widget (ui, "fuel_spin"));
  new_dlg->price_spin = GTK_SPIN_BUTTON (gui_glade_xml_get_widget (ui, "price_spin"));
  new_dlg->comment_entry = GTK_ENTRY (gui_glade_xml_get_widget (ui, "comment_entry"));
  new_dlg->ok_button = GTK_BUTTON (gui_glade_xml_get_widget (ui, "ok_button"));
  new_dlg->cars_list_store = gtk_list_store_new (N_CAR_COLUMNS,
                                                 G_TYPE_STRING,
						 G_TYPE_INT);

  /* store dialog struct as user data so it can be deleted on window destruction */
  g_object_set_data_full (G_OBJECT (dlg), "user_data", new_dlg, destroy_user_data);

  g_object_set (gui_glade_xml_get_widget (ui, "odometer_unit_label"),
                "label", length_unit_short,
		NULL);

  g_object_set (gui_glade_xml_get_widget (ui, "fuel_unit_label"),
                "label", capacity_unit_short,
		NULL);

  g_object_set (gui_glade_xml_get_widget (ui, "price_unit_label"),
                "label", monetary_unit_short,
		NULL);

  gtk_widget_set_sensitive (GTK_WIDGET (new_dlg->ok_button), FALSE);

  /* assign model */
  g_object_set (new_dlg->car_entry,
                "model", new_dlg->cars_list_store,
                NULL);

  glade_xml_signal_connect_data (ui,
                                 "car_entry_changed",
                                 G_CALLBACK (car_entry_changed),
                                 new_dlg);

  /* populate the combobox and set the active line */
  cars_list = mileage_db_get_cars();
  if (active_car)
    populate_cars_combo_box (GTK_COMBO_BOX (new_dlg->car_entry), cars_list, active_car->id);
  else
    populate_cars_combo_box (GTK_COMBO_BOX (new_dlg->car_entry), cars_list, -1);
  car_slist_free (cars_list);
  
  glade_xml_signal_connect_data (ui,
                                 "ok_clicked",
                                 G_CALLBACK (ok_clicked),
                                 new_dlg);

  glade_xml_signal_connect_data (ui,
                                 "cancel_clicked",
                                 G_CALLBACK (cancel_clicked),
                                 new_dlg);
}
