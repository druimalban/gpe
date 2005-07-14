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

#ifndef __GPE_MILEAGE_CAR_H__
#define __GPE_MILEAGE_CAR_H__

#include <glib.h>

#define CARS_SCHEMA "create table cars (car_id INTEGER PRIMARY KEY, description TEXT)"
#define CARS_TEST "select car_id,description from cars"

typedef struct car_s
{
  gint id;
  gchar *description;
} car_t;


/* Allocate memory for car structure, set initial values,
   and return pointer to it.  'description' is copied using g_strdup().  */
car_t *car_new (gint id,
                const gchar *description);

/* Free car structure and desciption string if non-NULL.  */
void car_free (car_t *car);

/* Output car values on stdout.  */
void car_output (car_t *car);

/* Free a GSList of car_t */
void car_slist_free (GSList *list);

/* Compare two car_t*. Don't use this for sorting! */
gint car_compare (car_t *a, car_t *b);

#endif /* __GPE_MILEAGE_CAR_H__ */
