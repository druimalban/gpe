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
		free (o->data);
		free (o);
	}
}

struct package *read_attrib (FILE *inp)
{
	struct package *p=NULL;
	char temp[1024];
	char c, *t;

	p = package_new ();

	/* Read the name */
	t = temp;
	while ((*t++ = getc (inp)) != '=' && t - temp < 1000 && !feof (inp));
	*--t = 0;
	p->name = (char *) strdup (temp);

	/* Read the data */
	t = temp;
	if ((c = getc (inp)) == '\"')
	{
		/* Quoted text */
		while ((*t++ = getc (inp)) != '\"' && t - temp < 1000 && !feof(inp))
			;
		*--t = 0;
	} else
	{
		*t++ = c;
		c = getc (inp);
		while (c != ' ' && c != '\t' && c != '\n' && c != '\\' && t - temp < 1000 && !feof(inp))
		{
			*t++ = c;
			c = getc (inp);
		}
		*t = 0;
		if (c == '\\')
			getc (inp);
	}
	p->data = (char *) strdup (temp);

	return p;
}

struct package *read_menu (FILE *inp)
{
	struct package *r=NULL, *p=NULL;
	char temp[1024];
	char c, *t;

	/* Find the beginning of the entry */
	while (!feof (inp) && isspace(c = getc (inp)))
		;

	if (feof (inp))
		return NULL;

	if (c != '?')
		return NULL;

	r = p = package_new ();

	/* Read the name */
	t = temp;
	while ((c = getc (inp)) != '(' && t - temp < 1000 && !feof(inp))
		*t++ = c;
	*t = 0;
	p->name = (char *) strdup (temp);

	/* Read the data */
	t = temp;
	while ((c = getc (inp)) != ')' && t - temp < 1000 && !feof(inp))
		*t++ = c;
	*t = 0;
	p->data = (char *) strdup (temp);

	/* Swallow the ':' */
	getc (inp);

	while (!feof (inp))
	{
		c = getc (inp);
		if (feof (inp))
			return r;

		switch (c)
		{
		case '\\':
			getc (inp);
		case '\t':
		case ' ':
			continue;
		case '\n':
			return r;
		default:
			ungetc (c, inp);
			p->next = read_attrib (inp);
			if (p->next)
				p = p->next;
			break;
		}
	}

	return r;
}

/* Read the file into a package struct.
   NOTE that this only loads *one* item
   from the menu file. There could be more!
   we have to watch out for this when we save.
   (FIXME :)
*/
struct package *package_read (char *fn)
{
	struct package *p, *r;
	FILE *inp;
	struct stat buf;

	if (!fn)
		return NULL;

	if (stat (fn, &buf) != 0)
	{
		/* File doesn't exist */
		return NULL;
	} else {
		/* Make sure it's a file, not (eg.) a directory */
		if (!S_ISREG(buf.st_mode))
			return NULL;
	}

	inp = fopen (fn, "r");

	if (!inp)
		return NULL;
	if (feof (inp)) {
		fclose (inp);
		return NULL;
	}

	p = read_menu (inp);
	if (!p) {
		fclose (inp);
		return NULL;
	}
	/* Prepend the filename of the package */
	r = package_new ();
	r->name = (char *) strdup ("file");
	r->data = (char *) strdup (fn);
	r->next = p;

	fclose (inp);

	return r;
}

void package_read_to (char *fn, void (* add_cb) (struct package *))
{
	struct package *p, *r;
	FILE *inp;
	struct stat buf;

	if (!fn)
		return;

	if (stat (fn, &buf) != 0)
	{
		/* File doesn't exist */
		return;
	} else {
		/* Make sure it's a file, not (eg.) a directory */
		if (!S_ISREG(buf.st_mode))
			return;
	}

	inp = fopen (fn, "r");

	if (!inp)
		return;
	while (!feof (inp))
	{
		p = read_menu (inp);
		if (!p) {
			fclose (inp);
			return;
		}

		/* Prepend the filename of the package */
		r = package_new ();
		r->name = (char *) strdup ("file");
		r->data = (char *) strdup (fn);
		r->next = p;

		add_cb (p);
	}
	fclose (inp);
}

void package_save_append (struct package *p, char *fn)
{
	FILE *out;
	struct package *t;

	if (!fn)
		fn = package_get_data (p, "file");
	if (!fn)
	{
		fprintf (stderr, "Can't write package, no filename given!\n");
		return;
	}

	out = fopen (fn, "w");
	if (!out)
	{
		perror ("package_save()");
		return;
	}

	t = p;
	while (t)
	{
		if (!strcmp (t->name, "package"))
			fprintf (out, "?package(%s):\\\n", t->data);
		else if (!strcmp (t->name, "file"))
			;
		else
			fprintf (out, "\t%s=\"%s\"%s\n", t->name, t->data, t->next ? "\\" : "");
		t=t->next;
	}

	fclose (out);
}

void package_save (struct package *p, char *fn)
{
	FILE *outp;

	/* Wipe out the old file */
	outp = fopen (fn, "w");
	if (!outp)
		return;
	fclose (outp);

	/* Write the new one */
	package_save_append (p, fn);
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
			p->data = (char*) strdup (data);
			return;
		}
		op=p;
		p=p->next;
	}
	/* We aren't replacing an entry, tack it on the end */
	op->next = package_new ();
	op->next->name = (char*) strdup (name);
	op->next->data = (char*)strdup (data);
}

int package_compare (struct package *a, struct package *b)
{
	char *n1, *n2;
	n1 = package_get_data (a, "title");
	n2 = package_get_data (b, "title");
	return strcasecmp (n1, n2);
}
