#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "playlist.h"

GSList *list;

void cb_update_length (struct playlist_entry *p, long len)
{
}

struct playlist_entry *playlist_add (char *fn)
{
	struct playlist_entry *p;

	fprintf (stderr, "Checking %s\n", fn);
	p = (struct playlist_entry *) malloc (sizeof (struct playlist_entry));
	p->plugin = plugin_input_find_by_file (fn);
	if (!p->plugin)
	{
		fprintf (stderr, "Dropping\n");
		free (p);
		return NULL;
	}
	p->filename = (char*) g_strdup (fn);
	p->title = p->plugin->get_tag (fn, "title");
	p->length = -1;
	if (p->plugin->get_length)
		p->length = p->plugin->get_length (fn);

	list = g_slist_append (list, p);

	/* Run after the row is created, just in case it returns too quickly */
	if (p->plugin->get_length_to)
		p->plugin->get_length_to (fn, (void*)cb_update_length, p);

	return p;
}

struct playlist_entry *playlist_get_nth (int n)
{
	GSList *list = g_slist_nth (list, n);
	return list ? list->data : NULL;
}

void playlist_remove (int n)
{
	GSList *l = g_slist_nth (list, n);

	if (l)
	{
		struct playlist_entry *p = l->data;
		list = g_slist_remove (list, l);
		if (p->title)
			g_free (p->title);
		if (p->filename)
			g_free (p->filename);
		free (p);
	}
}
