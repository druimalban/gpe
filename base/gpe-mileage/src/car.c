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

#include <glib.h>
#include "car.h"

car_t *
car_new (gint id,
         const gchar *description)
{
  car_t *car = g_new0(car_t, 1);

  car->id = id;
  car->description = g_strdup (description);
  
  return car;
}

void
car_free (car_t *car)
{
  if (car->description)
    {
      g_free(car->description);
      car->description = NULL;
    }
  
  g_free(car);
}

void
car_output (car_t *car)
{
  g_print("id: %d, description: %s\n",
          car->id, car->description);
}

void
car_slist_free (GSList *list)
{
  GSList *iter;
  
  for (iter = list; iter; iter = iter->next)
    car_free ((car_t *)(iter->data));
  
  g_slist_free (list);
}

gint
car_compare (car_t *a, car_t *b)
{
  if (a->id > b->id)
    return 1;
  if (a->id < b->id)
    return 2;
  return 0;
}
