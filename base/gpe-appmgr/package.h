/* gpe-appmgr - a program launcher

   Copyright 2002 Robert Mibus;

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

/* Read & write Debian-style menu entries */

#ifndef PACKAGE_H
#define PACKAGE_H

struct package_group
{
  gchar *name;
  GList *items;
};

struct package
{
  char *name;
  char *data;
  struct package *next;
};

/* Creates a new package structure, filled with NULLs.
   probably not really useful to most people */
struct package *package_new ();

/* Reading
   Either read one menu entry per file via package_read(),
   or use a callback via package_read_to() for multiple
   entries per file */
struct package *package_read (char *fn);
void package_read_to (char *fn, void (* add_cb) (struct package *));

/* Saving
   package_save() overwrites any current file with that name.
   package_save_append() only appends to the file. To write
   multiple menu entries, use package_save() for the first
   and package_save_append() for any others */
void package_save (struct package *p, char *fn);
void package_save_append (struct package *p, char *fn);

/* For getting/setting data about the package */
char *package_get_data (struct package *p, char *name);
void package_set_data (struct package *p, char *name, char *data);

/* Free'ing the package and its data */
void package_free (struct package *p);

/* Mostly a convenience function for alphabetically sorting
   GLists of packages. Runs strcasecmp() on the two titles */
int package_compare (struct package *a, struct package *b);

/* Load a .desktop file */
struct package *package_from_dotdesktop (char *filename, char *lang);

#endif
