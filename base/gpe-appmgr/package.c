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

/* Read debian-style menus */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h> /* for isspace() */

/* For stat() */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* For .desktop files */
#include <dotdesktop.h>

#include "package.h"

struct package *package_new ()
{
	struct package *p;

	p = (struct package *) malloc (sizeof (struct package));
	p->next = NULL;
	p->name = p->data = NULL;

	return p;	
}

void package_free (struct package *p)
{
	struct package *o;

	while (p)
	{
		o = p;
		p = p->next;
		free (o->name);
		if (o->data)
			free (o->data);
		free (o);
	}
}

char *package_get_data (struct package *p, char *name)
{
	while (p)
	{
		if (!strcmp (name, p->name))
			return p->data;
		p=p->next;
	}
	return NULL;
}

void package_set_data (struct package *p, char *name, char *data)
{
	struct package *op=NULL;
	while (p)
	{
		if (!strcmp (name, p->name))
		{
			free (p->data);
			if (data)
				p->data = (char*) strdup (data);
			else
				p->data = NULL;
			return;
		}
		op=p;
		p=p->next;
	}
	/* We aren't replacing an entry, tack it on the end */
	op->next = package_new ();
	op->next->name = (char*) strdup (name);
	if (data)
		op->next->data = (char*) strdup (data);
	else
		op->next->data = NULL;
}

int package_compare (struct package *a, struct package *b)
{
	char *n1, *n2;
	n1 = package_get_data (a, "title");
	n2 = package_get_data (b, "title");
	return strcasecmp (n1, n2);
}

struct package *package_from_dotdesktop (char *filename, char *lang)
{
	DotDesktop *dd;
	struct package *p;

	fprintf (stderr, "Loading %s\n", filename);
	dd = dotdesktop_new_from_file(filename, lang, "[Desktop Entry]");

	if (dd == NULL)
		return NULL;

	// want type == Application
	{
		char *t = dotdesktop_get (dd, "Type");
		if (t == NULL || strcmp(t, "Application"))
		{
			dotdesktop_free (dd);
			return NULL;
		}
	}

	p = package_new ();
	p->name = strdup ("dotdesktop");

	package_set_data (p, "title", dotdesktop_get (dd, "Name"));
	package_set_data (p, "command", dotdesktop_get (dd, "Exec"));
	package_set_data (p, "icon", dotdesktop_get (dd, "Icon"));
	package_set_data (p, "section", "Others");
	package_set_data (p, "Categories", dotdesktop_get (dd, "Categories"));

	/* Hackish mapping onto familiar menu policy groups.
	   TODO: Make this configurable */
	{
		struct mapping {
			char *from, *to;
		} mappings[] = {
			{"Game", "Games"},
			{"Utility", "Utilities"},
			{"PIM", "PIM"},
			{"AudioVideo", "Audio"},
			// {"Graphics", "???"},
			{"System", "Configuration"},
			{"SystemSetup", "Configuration"},
			{"Settings", "Configuration"},
			{"Amusement", "Games"},
			{"Office", "Office"},
			{"Internet", "Internet"},
			{NULL, NULL}
		};
		char *categories = dotdesktop_get (dd, "Categories");
		int i;
		for (i=0;mappings[i].from != NULL;i++)
		{
			char *from=g_strdup_printf ("%s;", mappings[i].from);
			if (strstr(categories, from))
			{
			  //printf ("Found: %s\n", mappings[i].to);
			  package_set_data (p, "section", mappings[i].to);
			  break;
			}
			g_free (from);
		}
	}

	dotdesktop_free (dd);

	return p;
}
