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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>		/* for getenv() */

#include <glib.h>
#include <sqlite.h>

#include "mileage_db.h"
#include "fuel_item.h"
#include "car.h"

/* SQLite database connection.  */
static sqlite *sqlite_db;

/* Path to db file relative to $HOME.  */
static const char *rel_db_file_path = "/.gpe/mileage";

/* SQLite debug callback function.  */
static int
debug_callback (void *arg, int argc, char **argv, char **names)
{
  int i;

  fprintf (stdout, "argc=%d\n", argc);

  for (i = 0; i < argc; i++)
    fprintf (stdout, "%s%s", argv[i], i < argc - 1 ? " | " : "\n");

  return 0;
}

/* Open SQLite database and create initial tables if necessary.  */
int
mileage_db_open (void)
{
  static const char *schema1_str = FUELITEMS_SCHEMA;
  static const char *schema2_str = CARS_SCHEMA;

  const char *home = getenv ("HOME");
  char *db_file_path;
  char *err;
  size_t len;

  if (home == NULL)
    home = "";
  len = strlen (home) + strlen (rel_db_file_path) + 1;
  db_file_path = g_malloc0 (len);
  strcpy (db_file_path, home);
  strcat (db_file_path, rel_db_file_path);

  sqlite_db = sqlite_open (db_file_path, 0, &err);
  g_free (db_file_path);
  if (sqlite_db == NULL)
    {
      fprintf (stderr, "Error opening db: %s\n", err);
      free (err);
      return -1;
    }

  /* Create fuelitems table. Ignore Errors.  */
  sqlite_exec (sqlite_db, schema1_str, NULL, NULL, &err);

  if (SQLITE_OK !=
      sqlite_exec (sqlite_db,
		   FUELITEMS_TEST,
		   NULL, NULL, &err))
    {
      fprintf (stderr, "Error creating db: %s\n", err);
      free (err);
      return -1;
    }

  /* Create cars table. Ignore Errors.  */
  sqlite_exec (sqlite_db, schema2_str, NULL, NULL, &err);

  if (SQLITE_OK !=
      sqlite_exec (sqlite_db,
		   CARS_TEST,
		   NULL, NULL, &err))
    {
      fprintf (stderr, "Error creating db: %s\n", err);
      free (err);
      return -1;
    }

  return 0;
}

/* Close SQLite db.  Misc cleanup.  */
void
mileage_db_close (void)
{
  sqlite_close (sqlite_db);
}

/* Callback function for SQlite to create a fuel_item.  */
int
create_fuel_item_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 7)
    {
      fuel_item_t **f = (fuel_item_t **) arg;
      
      *f = fuel_item_new(atoi(argv[0]), /* id */
                         atoi(argv[1]), /* car_id */
                         atoi(argv[2]), /* date (FIXME: Does this work?) */
                         strtod(argv[3], NULL), /* odometer_reading */
                         strtod(argv[4], NULL), /* fuel */
                         strtod(argv[5], NULL), /* price (FIXME: Does this work?) */
                         argv[6] /* comment */);
    }

  return 0;
}

/* Search the db for a fuel_item by id.  */
fuel_item_t *
mileage_db_get_fuel_item_by_id (int id)
{
  fuel_item_t *f = NULL;
  
  if (id < 0)
    return NULL;

  sqlite_exec_printf (sqlite_db, "select * from fuelitems where fuel_id=%d",
                      create_fuel_item_callback, &f, NULL, id);
  
  return f;
}

int
add_fuel_item_to_list_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 7)
    {
      GSList **list = (GSList ** ) arg;
      fuel_item_t *f = fuel_item_new(atoi(argv[0]), /* id */
                                     atoi(argv[1]), /* car_id */
                                     atoi(argv[2]), /* date (FIXME: Does this work?) */
                                     strtod(argv[3], NULL), /* odometer_reading */
                                     strtod(argv[4], NULL), /* fuel */
                                     strtod(argv[5], NULL), /* price (FIXME: Does this work?) */
                                     argv[6] /* comment */);
      
      *list = g_slist_prepend (*list, f);
    }
  return 0;
}

/* Return a list of all fuel items belonging to a specific car.  */
GSList *
mileage_db_get_fuel_items_by_car_id (gint car_id)
{
  GSList *list = NULL;
  
  if (car_id > -1)
    sqlite_exec_printf (sqlite_db, "SELECT * FROM fuelitems WHERE car_id=%d ORDER BY odometer_reading",
                        add_fuel_item_to_list_callback, &list, NULL, car_id);
  else
    sqlite_exec_printf (sqlite_db, "SELECT * FROM fuelitems ORDER BY odometer_reading",
                        add_fuel_item_to_list_callback, &list, NULL);
  
  /* Reverse the list to restore correct ordering.  */
  list = g_slist_reverse (list);
  
  return list;
}

/* Add a fuel_item to the db.  */
gboolean
mileage_db_add_fuel_item (fuel_item_t *f)
{
  gint res;

  if (!f)
    return FALSE;

  /*
  g_print ("Inserting ");
  fuel_item_output (f);
  */
  
  if (f->id > 0)
    res = sqlite_exec_printf (sqlite_db, "insert into fuelitems values "
                              "(%d, %d, %d, %f, %f, %f, %Q)",
                              NULL, NULL, NULL,
                              f->id, f->car_id, f->date, f->odometer_reading, f->fuel, f->price, f->comment);
  else
    res = sqlite_exec_printf (sqlite_db, "insert into fuelitems values "
                              "(NULL, %d, %d, %.2f, %.2f, %.2f, %Q)",
                              NULL, NULL, NULL,
                              f->car_id, f->date, f->odometer_reading, f->fuel, f->price, f->comment);

  return res == SQLITE_OK;
}

gboolean
mileage_db_delete_fuel_item_by_id (gint id)
{
  gint res;

  res = sqlite_exec_printf (sqlite_db, "delete from fuelitems where fuel_id=%d",
                            NULL, NULL, NULL,
			    id);
  return res == SQLITE_OK;
}

/* Callback function for SQlite to create a car.  */
int
create_car_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 2)
    {
      car_t **c = (car_t **) arg;
      
      *c = car_new(atoi(argv[0]),    /* id */
                   argv[1]);         /* description */
    }

  return 0;
}

/* Search the db for a car by id.  */
car_t *
mileage_db_get_car_by_id (int id)
{
  car_t *c = NULL;
  
  sqlite_exec_printf (sqlite_db, "select * from cars where car_id=%d",
                      create_car_callback, &c, NULL, id);
  
  return c;
}

/* Search the db for a car by description.  */
car_t *
mileage_db_get_car_by_description (const gchar* description)
{
  car_t *c = NULL;

  if (!description)
    return NULL;
  
  sqlite_exec_printf (sqlite_db, "select * from cars where description=%Q",
                      create_car_callback, &c, NULL, description);
  
  return c;
}

int
add_car_to_list_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 2)
    {
      GSList **list = (GSList ** ) arg;
      car_t *c = car_new(atoi(argv[0]),          /* id */
                         argv[1]);               /* description */
      
      *list = g_slist_prepend (*list, c);
    }
  return 0;
}

/* Return a list of all cars.  */
GSList *
mileage_db_get_cars ()
{
  GSList *list = NULL;
  
  sqlite_exec_printf (sqlite_db, "SELECT * FROM cars ORDER BY description",
                      add_car_to_list_callback, &list, NULL);
  
  /* Reverse the list to restore correct ordering.  */
  list = g_slist_reverse (list);
  
  return list;
}

/* Add a car to the db.  Return id of the car or -1 in case of an error.  */
gint
mileage_db_add_car (car_t *c)
{
  gint res, new_id;
  car_t *new_car;

  if (!c)
    return -1;
  
  /*
  g_print ("Inserting ");
  car_output (c);
  */

  if (c->id > -1)
    res = sqlite_exec_printf (sqlite_db, "insert into cars values "
                              "(%d, %Q)",
                              NULL, NULL, NULL,
                              c->id, c->description);
  else
    res = sqlite_exec_printf (sqlite_db, "insert into cars values "
                              "(NULL, %Q)",
                              NULL, NULL, NULL,
                              c->description);

  if (res != SQLITE_OK)
    return -1;

  if (c->id > -1)
    return c->id;

  new_car = mileage_db_get_car_by_description (c->description);
  if (!new_car)
    /* this shouldn't happen */
    return -1;
  new_id = new_car->id;
  car_free (new_car);
  
  return new_id;
}

gboolean
mileage_db_delete_car_by_id (gint id)
{
  gint res;

  res = sqlite_exec_printf (sqlite_db, "delete from fuelitems where car_id=%d",
                            NULL, NULL, NULL,
			    id);
  if (res != SQLITE_OK)
    return FALSE;

  res = sqlite_exec_printf (sqlite_db, "delete from cars where car_id=%d",
                            NULL, NULL, NULL,
			    id);
  return res == SQLITE_OK;
}

/* test code follows.  */
#if 0
int
main (void)
{
  char *err;
  int result;
  fuel_item_t *f;
  
  mileage_db_open ();

  result = sqlite_exec (sqlite_db, "select * from fuelitems",
			debug_callback, NULL, &err);
  if (result != SQLITE_OK)
    {
      fprintf (stderr, "Error #%d retrieving data: %s\n", result, err);
      free (err);
    }

  f = mileage_db_get_fuel_item_by_id (1);
  if (f)
    {
      fuel_item_output (f);
      fuel_item_free (f);
      f = NULL;
    }

  f = fuel_item_new (-1, 2, 1098028925, 280.6, 42.8, 50.0, "foom");
  mileage_db_add_fuel_item (f);
  fuel_item_free (f);
    
  mileage_db_close ();
  return 0;
}
#endif
