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

#ifndef __GPE_MILEAGE_FUEL_ITEM_H__
#define __GPE_MILEAGE_FUEL_ITEM_H__

#include <glib.h>
#include <time.h>

#define FUELITEMS_SCHEMA "create table fuelitems (fuel_id INTEGER PRIMARY KEY, car_id INTEGER, date INTEGER, odometer_reading FLOAT, fuel FLOAT, price FLOAT, comment TEXT)"
#define FUELITEMS_TEST "select fuel_id,date,odometer_reading,price,comment from fuelitems"

typedef struct fuel_item_s
{
  gint id;
  gint car_id;
  time_t date;
  gdouble odometer_reading;
  gdouble fuel;
  gdouble price;
  gchar *comment;
} fuel_item_t;


/* Allocate memory for fuel_item structure, set initial values,
   and return pointer to it.  'comment' is copied using g_strdup().  */
fuel_item_t *fuel_item_new (gint id,
                            gint car_id,
                            time_t date,
                            gdouble odometer_reading,
                            gdouble fuel,
                            gdouble price,
                            const gchar *comment);

/* Free fuel_item structure and comment string if non-NULL.  */
void fuel_item_free (fuel_item_t *f);

/* Output fuel_item values on stdout.  */
void fuel_item_output (fuel_item_t *t);

/* Free a GSList of fuel_item_t nodes.  */
void fuel_item_slist_free (GSList *list);

#endif /* __GPE_MILEAGE_FUEL_ITEM_H__ */
