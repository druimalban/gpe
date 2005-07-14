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
#include "fuel_item.h"

#define BUFSIZE 30

fuel_item_t *
fuel_item_new (gint id,
               gint car_id,
               time_t date,
               gdouble odometer_reading,
               gdouble fuel,
               gdouble price,
               const gchar *comment)
{
  fuel_item_t *f = g_new0(fuel_item_t, 1);

  f->id = id;
  f->car_id = car_id;
  f->date = date;
  f->odometer_reading = odometer_reading;
  f->fuel = fuel;
  f->price = price;
  f->comment = g_strdup(comment);
  
  return f;
}

void
fuel_item_free (fuel_item_t *f)
{
  if (f->comment)
    {
      g_free(f->comment);
      f->comment = NULL;
    }
  
  g_free(f);
}

void
fuel_item_output (fuel_item_t *f)
{
  struct tm tm;
  char *buf;
  
  buf = g_malloc0 (BUFSIZE);
  localtime_r (&(f->date), &tm);
  strftime (buf, BUFSIZE, "%c", &tm);
  
  g_print("id: %d, car_id: %d, date: %s, odometer_reading: %f, fuel: %f, price: %f, comment: %s\n",
          f->id, f->car_id, buf, f->odometer_reading, f->fuel, f->price, f->comment);
}

void
fuel_item_slist_free (GSList *list)
{
  GSList *iter;
  
  for (iter = list; iter; iter = iter->next)
    fuel_item_free ((fuel_item_t *)(iter->data));
  
  g_slist_free (list);
}
