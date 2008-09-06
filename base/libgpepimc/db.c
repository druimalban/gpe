/*
 * Copyright (C) 2001, 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 *               2006 Florian Boor <florian.boor@kernelconcepts.de>
 *               2007 Graham R. Cobb <g+gpe@cobb.uk.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <sqlite.h>

#include <gpe/errorbox.h>

#include "gpe/pim-categories.h"

#include "internal.h"

#define _(x) (x)

#define DB_NAME "/.gpe/categories"

static const char *schema_old_tmp_str = "create temporary table old_category (id INTEGER PRIMARY KEY, description TEXT);";
static const char *schema_str = "create table category (id INTEGER PRIMARY KEY, description TEXT, colour TEXT);";

static GSList *categories;
static sqlite *db;

static int
load_one (void *arg, int argc, char **argv, char **names)
{
  if (argc == 3)
    {
      GSList **list = (GSList **) arg;
      struct gpe_pim_category *c = g_malloc0 (sizeof (c));

      c->id = atoi (argv[0]);
      c->name = g_strdup (argv[1]);
      if (argv[2] && strcmp (argv[2], "(NULL)"))
        c->colour = g_strdup (argv[2]);
      *list = g_slist_prepend (*list, c);
    }

  return 0;
}

/* check categories table and update to new scheme if necessary */
static void
check_table_update (void)
{
  gint r;
  gchar *err = NULL;

  /* check if we have the colour field */
  r = sqlite_exec (db, "select colour from category", NULL, NULL, &err);

  if (r) /* r > 0 indicates a failure, need to recreate that table with new schema */
    {
      g_free (err);
      r = sqlite_exec (db, "begin transaction", NULL, NULL, &err);
      if (r)
        goto error;
      r = sqlite_exec (db, schema_old_tmp_str, NULL, NULL, &err);
      if (r)
        goto error;
      r = sqlite_exec (db, "insert into old_category select id,description from category", NULL, NULL, &err);
      if (r)
        goto error;
      r = sqlite_exec (db, "drop table category", NULL, NULL, &err);
      if (r)
        goto error;
      r = sqlite_exec (db, schema_str, NULL, NULL, NULL);
      if (r)
       goto error;
      r = sqlite_exec (db, "insert into category select id,description,NULL from old_category", NULL, NULL, &err);
      if (r)
        goto error;
      r = sqlite_exec (db, "drop table old_category", NULL, NULL, &err);
      if (r)
        goto error;
      r = sqlite_exec (db, "commit transaction", NULL, NULL, &err);
      if (r)
        goto error;
      r = sqlite_exec (db, "vacuum", NULL, NULL, &err);
      if (r)
        goto error;

      return;

    error:
      {
        gpe_error_box_fmt ("Couldn't convert database data to new format: %s",
                           err);
        g_free (err);
        sqlite_exec (db, "rollback transaction", NULL, NULL, &err);
      }
    }
}

static void gpe_pim_category_free(struct gpe_pim_category *c)
{
    g_free((gpointer)c->name);
    g_free((gpointer)c->colour);

    g_free(c);

    categories = g_slist_remove(categories, c);
}

/* Reload (or initially load) the categories from the database */
static gboolean 
gpe_pim_categories_refresh (void)
{
  /* Note: this routine is designed to be called to refresh the list whenever the PIM
     application is about to create a new object (event, contact, etc.).
     It should really check to see if the database has been modified since it was last
     refeshed, but as category lists are normally quite small it doesn't.  

     In future, if this is not acceptable, it would be possible to add a table to record
     the last database update time and check that before discarding and reloading the
     categories */
  gchar *err;

  while (categories) {
    gpe_pim_category_free((struct gpe_pim_category *)categories->data);
  }

  if (sqlite_exec (db, "select id,description,colour from category order by description desc",
                   load_one, &categories, &err))
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }
  
  return TRUE;
}

gboolean 
gpe_pim_categories_init (void)
{
  static int run_once;

  if (run_once)
    return TRUE;
  run_once = TRUE;

  gchar *err;
  gchar *buf;
  size_t len;
  gchar *home = getenv ("HOME");
  if (home == NULL)
    home = "";
  
  len = strlen (home) + strlen (DB_NAME) + 1;
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, DB_NAME);
  
  db = sqlite_open (buf, 0, &err);
  g_free (buf);
	
  if (db == NULL) 
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }
  
  /* make sure table exists */
  sqlite_exec (db, schema_str, NULL, NULL, NULL);
  
  /* update table layout if necessary */
  check_table_update ();

  return gpe_pim_categories_refresh();
}

GSList *
gpe_pim_categories_list (void)
{
  GSList *ids = NULL, *iter;

  gpe_pim_categories_refresh();

  for (iter = categories; iter; iter = iter->next)
    {
      struct gpe_pim_category *c = iter->data;

      ids = g_slist_append(ids, (gpointer)c->id);
    }

  return ids;
}

static struct gpe_pim_category *gpe_pim_category_by_name (const gchar *name) {
  GSList *iter;

  for (iter = categories; iter; iter = iter->next)
    {
      struct gpe_pim_category *c = iter->data;

      if (!strcasecmp (c->name, name))
        return c;
    }

  return NULL;
}

static struct gpe_pim_category *gpe_pim_category_by_id (gint id) {
  GSList *iter;

  for (iter = categories; iter; iter = iter->next)
    {
      struct gpe_pim_category *c = iter->data;

      if (c->id == id)
        return c;
    }

  return NULL;
}

const gchar *
gpe_pim_category_name (gint id)
{
  struct gpe_pim_category *c = gpe_pim_category_by_id(id);

  if (c) return c->name;

  return NULL;
}

gint
gpe_pim_category_id (const gchar *name)
{
  struct gpe_pim_category *c = gpe_pim_category_by_name(name);

  if (c) return c->id;

  return -1;
}

/**
 * gpe_pim_category_colour:
 * @id: Category id
 *
 * Get the colour assigned to the given category.
 *
 * Returns: Colour string.
 */
const gchar *
gpe_pim_category_colour (gint id)
{
  struct gpe_pim_category *c = gpe_pim_category_by_id(id);

  if (c) return c->colour;

  return NULL;
}

gboolean
gpe_pim_category_new (const gchar *name, gint *id)
{
  gchar *err;
  gint r;
  struct gpe_pim_category *c;

  r = gpe_pim_category_id(name);
  if (r >= 0)
    {
      // Category already exists -- pretend we created it
      *id = r;
      return TRUE;
    }
  
  r = sqlite_exec_printf (db, "insert into category values (NULL, '%q', NULL)",
                          NULL, NULL, &err, name);
  if (r)
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }
  
  *id = sqlite_last_insert_rowid (db);

  c = g_malloc0 (sizeof (c));

  c->id = *id;
  c->name = g_strdup (name);

  categories = g_slist_prepend (categories, c);
      
  return TRUE;
}

/**
 * gpe_pim_category_delete:
 * @c: Pim category to become deleted.
 *
 * Delete a PIM category. The category to delete is isentified by the id 
 * field in the given category.
 */
void 
gpe_pim_category_delete (gint id)
{
  struct gpe_pim_category *c = gpe_pim_category_by_id(id);

  sqlite_exec_printf (db, "delete from category where id='%d'", NULL, NULL, NULL, c->id);

  gpe_pim_category_free(c);
}

/**
 * gpe_pim_category_rename:
 * @id: Category id
 * @new_name: String
 *
 * Rename an existing category.
 *
 * Returns: %TRUE on success, %FALSE otherwise.
 */
gboolean
gpe_pim_category_rename (gint id, gchar *new_name)
{
  gchar *err;
  gint r;

  if (gpe_pim_category_by_name(new_name))
    {
      gpe_error_box ("Destination category already exists");
      return FALSE;
    }

  r = sqlite_exec_printf (db,
                          "update category set description = '%q' where id =%d",
                          NULL, NULL, &err, new_name, id);

  if (r)
    {
      gpe_error_box (err);
      g_free (err);
      return FALSE;
    }

  struct gpe_pim_category *c = gpe_pim_category_by_id(id);

  g_free((gpointer)c->name);
  c->name = g_strdup(new_name);

  return TRUE;
}

/**
 * gpe_pim_category_set_colour:
 * @id: Category id
 * @new_colour: Colour description.
 *
 * Set the colour descrition for a category. Valid strings are 
 * symbolic names like 'green' or HTML-like RGB values e.g. <quote>#11FF11</quote>
 *
 * Returns: %TRUE on success, %FALSE otherwise.
 */
gboolean
gpe_pim_category_set_colour (gint id, const gchar *new_colour)
{
  gchar *err;
  gint r;

  r = sqlite_exec_printf (db,
                          "update category set colour = '%q' where id =%d",
                          NULL, NULL, &err, new_colour, id);

  if (r)
    {
      gpe_error_box (err);
      g_free (err);
      return FALSE;
    }

  struct gpe_pim_category *c = gpe_pim_category_by_id(id);

  g_free((gpointer)c->colour);
  c->colour = g_strdup(new_colour);

  return TRUE;
}
