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

#ifndef __GPE_MILEAGE_MILEAGE_DB_H__
#define __GPE_MILEAGE_MILEAGE_DB_H__

#include <glib.h>
#include "fuel_item.h"
#include "car.h"

/* Open SQLite database and create initial tables if necessary.  */
int mileage_db_open (void);

/* Close SQLite db.  Misc cleanup.  */
void mileage_db_close (void);

/* Search the db for a fuel_item by id.  */
fuel_item_t *mileage_db_get_fuel_item_by_id (int id);

/* Return a list of all fuel items belonging to a specific car.  */
GSList *mileage_db_get_fuel_items_by_car_id (gint car_id);

/* Add a fuel_item to the db.  */
gboolean mileage_db_add_fuel_item (fuel_item_t *f);

/* Delete fuel item by id.  */
gboolean mileage_db_delete_fuel_item_by_id (gint id);

/* Search the db for a car by id.  */
car_t *mileage_db_get_car_by_id (int id);

/* Search the db for a car by description.  */
car_t *mileage_db_get_car_by_description (const gchar* description);

/* Return a list of all cars.  */
GSList *mileage_db_get_cars ();

/* Add a car to the db.  Return id of the car or -1 in case of an error.  */
gint mileage_db_add_car (car_t *c);

/* Delete a car by id.  Also deletes all associated fuel items.  */
gboolean mileage_db_delete_car_by_id (gint id);

#endif /*__GPE_MILEAGE_MILEAGE_DB_H__*/
