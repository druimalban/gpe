/* GPE Mileage
 * Copyright (C) 2004  Rene Wagner <rw@handhelds.org>
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
#include "new_fuel_item_dlg.h"
#include "preferences_dlg.h"
#include "gui.h"
#include "view-common.h"
#include "preferences.h"
#include "units.h"

#include "gpe-mileage-i18n.h"

enum {
  ODOMETER_COLUMN,
  FUEL_COLUMN,
  PRICE_COLUMN,
  FUEL_ID_COLUMN,
  N_FUEL_COLUMNS
};

const gchar *
column_headers[] = {
  N_("Odometer"),
  N_("Fuel"),
  N_("Price")
};

typedef struct main_window_s
{
  /* GUI elements */
  GtkWindow *window;
  GtkComboBox *cars_combobox;
  GtkLabel *average_mileage_label;
  GtkLabel *last_mileage_label;
  GtkTreeView *fuel_view;
  /* backing data structures */
  GtkListStore *fuel_list_store;
  GtkListStore *cars_list_store;
} main_window_t;

void
populate_fuel_list_store (GtkListStore *list_store, GSList *fuel_items)
{
  GtkTreeIter tree_iter;
  GSList *list_iter;
  const gchar* length_unit_short = units_short_description (UNITS_LENGTH, preferences_get_unit (UNITS_LENGTH));
  const gchar* capacity_unit_short = units_short_description (UNITS_CAPACITY, preferences_get_unit (UNITS_CAPACITY));
  const gchar* monetary_unit_short = units_short_description (UNITS_MONETARY, preferences_get_unit (UNITS_MONETARY));

  gtk_list_store_clear (list_store);
  for (list_iter = fuel_items; list_iter; list_iter = list_iter->next)
    {
      fuel_item_t *f = (fuel_item_t *)(list_iter->data);
      gchar *odometer_string = g_strdup_printf ("%.2f %s", f->odometer_reading, length_unit_short);
      gchar *fuel_string = g_strdup_printf ("%.2f %s", f->fuel, capacity_unit_short);
      gchar *price_string = g_strdup_printf ("%.2f %s", f->price, monetary_unit_short);
      
      gtk_list_store_append (list_store, &tree_iter);
      gtk_list_store_set (list_store, &tree_iter,
                          ODOMETER_COLUMN, odometer_string,
                          FUEL_COLUMN, fuel_string,
                          PRICE_COLUMN, price_string,
			  FUEL_ID_COLUMN, f->id,
                          -1);

      g_free(odometer_string);
      g_free(fuel_string);
      g_free(price_string);
    }
}

void
populate_cars_combo_box (GtkComboBox *box, GSList *cars_list, gint selected_car_id)
{
  GtkListStore *list_store = GTK_LIST_STORE (gtk_combo_box_get_model (box));
  GtkTreeIter cars_iter;
  GSList *list_iter;

  gtk_list_store_clear (list_store);
  if (NULL == cars_list)
    {
      gtk_list_store_append (list_store, &cars_iter);
      gtk_list_store_set (list_store, &cars_iter,
                          CAR_DESCRIPTION_COLUMN, _("No cars in DB"),
			  CAR_ID_COLUMN, -1,
                          -1);
    } 
  else
    {
      for (list_iter = cars_list; list_iter; list_iter = list_iter->next)
        {
          car_t *c = (car_t *)(list_iter->data);
          
          gtk_list_store_append (list_store, &cars_iter);
          gtk_list_store_set (list_store, &cars_iter,
                              CAR_DESCRIPTION_COLUMN, c->description,
			      CAR_ID_COLUMN, c->id,
                              -1);

	  if (c->id == selected_car_id)
	    gtk_combo_box_set_active_iter (box, &cars_iter);
        }
    }

  /* activate first row as a fallback.  */
  if (gtk_combo_box_get_active (box) < 0)
    gtk_combo_box_set_active (box, 0);
}

void
refresh (gpointer data, gint selected_car_id)
{
  main_window_t *main_window = (main_window_t *) data;

  GSList *cars_list = NULL;

  /* update cars_list from db */
  cars_list = mileage_db_get_cars ();

  /* populate the combobox and set the active line */
  populate_cars_combo_box (main_window->cars_combobox, cars_list, selected_car_id);
  car_slist_free (cars_list);
  
  /* the rest is done by the combobox_changed handler already */
}

void
delete_button_clicked (GtkWidget *widget, gpointer data)
{
  main_window_t *main_window = (main_window_t *) data;

  GtkTreeSelection *selection = gtk_tree_view_get_selection (main_window->fuel_view);
  GtkTreeIter iter;
  gboolean selected = FALSE;

  selected = gtk_tree_selection_get_selected (selection, NULL, &iter);
  if (selected)
    {
      gint selected_fuel_id = -1;

      gtk_tree_model_get (GTK_TREE_MODEL (main_window->fuel_list_store), &iter,
                          FUEL_ID_COLUMN, &selected_fuel_id,
			  -1);

      mileage_db_delete_fuel_item_by_id (selected_fuel_id);
      gtk_list_store_remove (main_window->fuel_list_store, &iter);
    }
  else
    {
      GtkWidget *dialog = NULL;
      selected = gtk_combo_box_get_active_iter (main_window->cars_combobox, &iter);
      if (selected)
        {
          gint active_car_id = -1;

	  gtk_tree_model_get (GTK_TREE_MODEL (main_window->cars_list_store), &iter,
	                      CAR_ID_COLUMN, &active_car_id,
			      -1);

         if (active_car_id)
	   {
	     gchar *car_description;
	     gint response;

	     gtk_tree_model_get (GTK_TREE_MODEL (main_window->cars_list_store), &iter,
	                         CAR_DESCRIPTION_COLUMN, &car_description,
				 -1);
	     dialog = gtk_message_dialog_new (GTK_WINDOW (main_window->window),
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_MESSAGE_QUESTION,
					      GTK_BUTTONS_YES_NO,
				              _("Delete entire dataset to car `%s'?"),
					      car_description);

	     response = gtk_dialog_run (GTK_DIALOG (dialog));
	     switch (response)
	       {
		 case GTK_RESPONSE_YES:
	            mileage_db_delete_car_by_id (active_car_id);
	            refresh (main_window, -1);
		    break;
		 default:
		    /* do nothing.  */
		    break;
	       }
	     gtk_widget_destroy (GTK_WIDGET (dialog));
	   }
	}
    }
}

void
new_button_clicked(GtkWidget *widget, gpointer data)
{
  main_window_t *main_window = (main_window_t *) data;

  car_t *active_car = NULL;
  GtkTreeIter iter;
  gboolean set = FALSE;

  set = gtk_combo_box_get_active_iter (main_window->cars_combobox, &iter);
  if (set)
    {
      gint active_car_id = -1;

      gtk_tree_model_get (GTK_TREE_MODEL (main_window->cars_list_store), &iter,
                                          CAR_ID_COLUMN, &active_car_id,
					  -1);
      active_car = mileage_db_get_car_by_id (active_car_id);
    }

  new_fuel_item_dlg_open (main_window->window, active_car, G_CALLBACK (refresh), main_window);
}

void
preferences_button_clicked (GtkWidget *widget, gpointer data)
{
  main_window_t *main_window = (main_window_t *) data;

  preferences_dlg_open (main_window->window);
}

gdouble
calculate_mileage (gdouble old, gdouble new, gdouble fuel)
{
  switch (preferences_get_unit (UNITS_MILEAGE))
    {
      case UNITS_MPG:
        return (new-old) / fuel;
      case UNITS_LP100KM:
      default: /* FIXME */
        return fuel / (new-old) * 100;
      
    }
}

gdouble
calculate_last_mileage (GSList *list)
{
  gdouble old, new, fuel;

  if (!list || !list->next)
    {
      return 0;
    }

  list = g_slist_reverse(list);

  old = ((fuel_item_t *)(list->next->data))->odometer_reading;
  new = ((fuel_item_t *)(list->data))->odometer_reading;
  fuel = ((fuel_item_t *)(list->data))->fuel;

  list = g_slist_reverse (list);

  return calculate_mileage (old, new, fuel);
}

gdouble
calculate_avg_mileage (GSList *list)
{
  GSList *list_iter;
  gint count = 0;
  gdouble old_odo, res = 0;

  for (list_iter=list; list_iter; list_iter = list_iter->next)
    {
      fuel_item_t *f = (fuel_item_t *)(list_iter->data);

      if (list_iter != list)
        {
      	  res += calculate_mileage (old_odo, f->odometer_reading, f->fuel);
	  count++;
	}
      old_odo = f->odometer_reading;
    }

  /* g_print ("res: %f; count: %d\n", res, count); */
  return count > 0 ? res / count : 0;
}

gboolean
update_stats (main_window_t* main_window, GSList* fuel_items_list)
{
  const gchar* mileage_unit_short_desc = units_short_description (UNITS_MILEAGE, preferences_get_unit (UNITS_MILEAGE));
  gchar* avg_mileage = g_strdup_printf ("%.2f %s", calculate_avg_mileage (fuel_items_list), mileage_unit_short_desc);
  gchar* last_mileage = g_strdup_printf ("%.2f %s", calculate_last_mileage (fuel_items_list), mileage_unit_short_desc);

  g_object_set (main_window->average_mileage_label,
                "label", avg_mileage,
		NULL);
  g_object_set (main_window->last_mileage_label,
                "label", last_mileage,
		NULL);

  /* free memory */
  g_free (avg_mileage);
  g_free (last_mileage);

  return TRUE;
}

void
cars_combobox_changed(GtkWidget *widget, gpointer data)
{
  GtkComboBox *box = GTK_COMBO_BOX (widget);
  main_window_t *main_window = (main_window_t *) data;

  GtkListStore *cars_store = GTK_LIST_STORE (gtk_combo_box_get_model (box));
  GSList *fuel_items_list = NULL;
  GtkTreeIter iter;
  gboolean set = FALSE;
  gint selected_car = -1;

  set = gtk_combo_box_get_active_iter (box, &iter);
  if (set)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (cars_store), &iter,
  	                  CAR_ID_COLUMN, &selected_car,
			  -1);
      if (selected_car)
        fuel_items_list = mileage_db_get_fuel_items_by_car_id (selected_car);
    }
  populate_fuel_list_store (main_window->fuel_list_store, fuel_items_list);
  update_stats (main_window, fuel_items_list);

  fuel_item_slist_free (fuel_items_list);
}

void
units_changed (UnitType type, gpointer data)
{
  main_window_t *main_window = (main_window_t *) data;
  
  cars_combobox_changed (GTK_WIDGET (main_window->cars_combobox), data);
}

gint
delete_event (GtkWidget * widget, GdkEvent event, gpointer data)
{
  /* just close the window */
  return FALSE;
}

void
gpe_mileage_exit (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

void
main_window_init (void)
{
  GladeXML *ui = gui_glade_xml_new ("main_window");
  /* GUI elements */
  GtkWindow *window = GTK_WINDOW (gui_glade_xml_get_widget (ui, "main_window"));
  GtkTreeView *view = GTK_TREE_VIEW (gui_glade_xml_get_widget (ui, "treeview1"));
  GtkComboBox *cars_combobox = GTK_COMBO_BOX (gui_glade_xml_get_widget (ui, "combobox1"));
  /* backing model stuff */
  GtkListStore *cars_list_store, *fuel_items_list_store;
  GtkTreeViewColumn *odo_column, *fuel_column, *price_column;
  GtkCellRenderer *text_renderer;
  /* actual data lists */
  GSList *cars_list = NULL, *fuel_items_list = NULL, *list_iter;  
  /* struct to pass stuff to callback handlers */
  main_window_t *main_window = g_new0(main_window_t, 1);
  
  cars_list_store = gtk_list_store_new (N_CAR_COLUMNS,
  					G_TYPE_STRING,
					G_TYPE_INT);
  g_object_set (cars_combobox,
                "model", cars_list_store,
                NULL);

  cars_list = mileage_db_get_cars();
  populate_cars_combo_box (cars_combobox, cars_list, -1);
  gtk_combo_box_set_active (cars_combobox, 0);

  fuel_items_list_store = gtk_list_store_new (N_FUEL_COLUMNS,
                                              G_TYPE_STRING,
                                              G_TYPE_STRING,
                                              G_TYPE_STRING,
					      G_TYPE_INT);

  if (cars_list)
    {
      fuel_items_list = mileage_db_get_fuel_items_by_car_id(((car_t *)(cars_list->data))->id);
      populate_fuel_list_store (fuel_items_list_store, fuel_items_list);
    }

  g_object_set (view,
                "model", fuel_items_list_store,
                NULL);

  text_renderer = gtk_cell_renderer_text_new();
  odo_column = gtk_tree_view_column_new_with_attributes (_(column_headers[ODOMETER_COLUMN]),
                                                         text_renderer,
                                                         "text", ODOMETER_COLUMN,
                                                         NULL);
  fuel_column = gtk_tree_view_column_new_with_attributes (_(column_headers[FUEL_COLUMN]),
                                                          text_renderer,
                                                          "text", FUEL_COLUMN,
                                                          NULL);
  price_column = gtk_tree_view_column_new_with_attributes (_(column_headers[PRICE_COLUMN]),
                                                           text_renderer,
                                                           "text", PRICE_COLUMN,
                                                           NULL);

  gtk_tree_view_append_column (view, odo_column);
  gtk_tree_view_append_column (view, fuel_column);
  gtk_tree_view_append_column (view, price_column);

  /* populate main_window struct */
  main_window->window = GTK_WINDOW (gui_glade_xml_get_widget (ui, "main_window"));
  main_window->cars_combobox = cars_combobox;
  main_window->average_mileage_label = GTK_LABEL (gui_glade_xml_get_widget (ui, "average_mileage_label"));
  main_window->last_mileage_label = GTK_LABEL (gui_glade_xml_get_widget (ui, "last_mileage_label"));
  main_window->cars_list_store = cars_list_store;
  main_window->fuel_list_store = fuel_items_list_store;
  main_window->fuel_view = view;

  update_stats (main_window, fuel_items_list);

  fuel_item_slist_free (fuel_items_list);

  preferences_notify_add (PREFERENCES_DOMAIN_UNITS, units_changed, main_window);

  glade_xml_signal_connect_data (ui,
                                 "cars_combobox_changed",
                                 G_CALLBACK (cars_combobox_changed),
                                 main_window);
  
  glade_xml_signal_connect_data (ui,
                                 "new_button_clicked",
                                 G_CALLBACK (new_button_clicked),
                                 main_window);

  glade_xml_signal_connect_data (ui,
                                 "delete_button_clicked",
                                 G_CALLBACK (delete_button_clicked),
                                 main_window);

  glade_xml_signal_connect_data (ui,
                                 "preferences_button_clicked",
                                 G_CALLBACK (preferences_button_clicked),
                                 main_window);

  /* Standard window signals */
  g_signal_connect (window, "delete-event", G_CALLBACK (delete_event), NULL);

  g_signal_connect (window, "destroy", G_CALLBACK (gpe_mileage_exit), NULL);

  /* show all */
  gtk_widget_show_all (GTK_WIDGET (window));
}
