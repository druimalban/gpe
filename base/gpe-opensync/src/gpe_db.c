#include "gpe_sync.h"

static int
fetch_uid_callback (void *arg, int argc, char **argv, char **names)
{
	if (argc == 1)
	{
		GSList **data = (GSList **)arg;

		*data = g_slist_prepend (*data, (void *)atoi (argv[0]));
	}

	return 0;
}

GSList *
fetch_uid_list (nsqlc *db, const gchar *query, ...)
{
	GSList *data = NULL;
	va_list ap;

	va_start (ap, query);

	nsqlc_exec_vprintf (db, query, fetch_uid_callback, &data, NULL, ap);

	va_end (ap);

	return data;
}

static int
fetch_callback (void *arg, int argc, char **argv, char **names)
{
	if (argc == 2)	{
		GSList **data = (GSList **) arg;
		gpe_tag_pair *p = g_malloc (sizeof (*p));

		// TODO: german umlauts are not converted correctly
		//  - bug of libnsqlc?
		// this has to be done, otherwise problems may
		// occur when converting the vcard to a string.
		argv[1] = g_locale_to_utf8(argv[1],-1,NULL,NULL,NULL);
		p->tag = g_strdup (argv[0]);
		p->value = g_strdup (argv[1]);

		*data = g_slist_prepend (*data, p);
	}

	return 0;
}

GSList *
fetch_tag_data (nsqlc *db, const gchar *query_str, guint id)
{
	GSList *data = NULL;

	nsqlc_exec_printf (db, query_str, fetch_callback, &data, NULL, id);

	return data;
}

gchar * get_tag_value(GSList *tags, gchar *tag)
{
	GSList *i;
	gpe_tag_pair *p;
	
	for(i = tags; i; i=i->next)
	{
		p = (gpe_tag_pair *)i->data;		
		if(!strcmp(p->tag, tag))
				return g_strdup(p->value);
	}
	return NULL;
}

gboolean
store_tag_data (nsqlc *db, const gchar *table, guint id, GSList *tags,
		gboolean delete)
{
	if (delete)
		nsqlc_exec_printf (db, "delete from '%q' where urn='%d'", NULL, NULL, NULL, table, id);

	while (tags)
	{
		gpe_tag_pair *p = tags->data;
		nsqlc_exec_printf (db, "insert into '%q' values (%d, '%q', '%q')", NULL, NULL, NULL, table, id, p->tag, p->value);
		tags = tags->next;
	}

	return TRUE;
}

void s_print_tags (GSList *tags, GString *str)
{
	GSList *i;
	gpe_tag_pair *p;

	for (i=tags; i; i=i->next)
	{
		p = i->data;
		g_string_append_printf(str, "%s = %s\n", p->tag, p->value);
	}
	
	return;
}

int get_new_urn (nsqlc *db)
{
	GSList *list, *i;
	int urn = 0;
	list = fetch_uid_list (db, "select distinct urn from contacts_urn");
	
	for (i = list; i; i = i->next)
	{
		if((int)i->data > urn)
			urn = (int)i->data;
	}

	return urn + 1;
}

static int
categories_callback (void *arg, int argc, char **argv, char **names)
{
	if (argc == 2)	{
		GSList **data = (GSList **) arg;
		gpe_tag_pair *p = g_malloc (sizeof (*p));

		// TODO: german umlauts are not converted correctly
		//  - bug of libnsqlc?
		// this has to be done, otherwise problems may
		// occur when converting the vcard to a string.
		argv[1] = g_locale_to_utf8(argv[1],-1,NULL,NULL,NULL);
		p->tag = g_strdup (argv[0]);
		p->value = g_strdup (argv[1]);

		*data = g_slist_prepend (*data, p);
	}

	return 0;
}

GSList *
fetch_categories (nsqlc *db)
{
	GSList *data = NULL;

	nsqlc_exec_printf (db, "select * from category", categories_callback, &data, NULL);

	return data;
}

/*! \brief Replaces the category numbers with their names
 *
 * \param tags		The tags which contain gpe_tag_pairs of tags
 * 			from the sqlite database.
 * \param categories	A List of the categories, also with gpe_tag_pairs,
 * 			where tag is the category number and value the
 * 			category name
 */
void tags_replace_category (GSList *tags, GSList *categories)
{
	GSList *i, *i2, *first_category = NULL, *delete_link = NULL;
	gpe_tag_pair *p, *p2;
	GString *s=NULL;
	gchar *category_name;

	i = tags;
	while (i)
	{
		p = i->data;
		if (!strcasecmp(p->tag, "category")) {
			category_name = NULL;

			// Now we have to loop through all categories
			// to find out the text for this category number.
			for (i2 = categories; i2; i2 = i2->next) {
				// p2 is a tag-value pair of the categories,
				// where tag is the number and value the
				// name of the category.
				p2 = i2->data;
				if (!strcasecmp(p2->tag, p->value)) {
					category_name = g_strdup (p2->value);
				}
			}
			if (first_category == NULL) {
				s = g_string_new (category_name);
				first_category = i;
			}
			else {
				// Okay, this is tricky: We want only *one*
				// category in our tag-list. For this, we
				// always mark any further links as the
				// delete_link an delete it at the end of the
				// while-loop.
				delete_link = i->data;
				g_string_append_printf (s, ",%s", category_name);
			}
		}
		i = i->next;
		if (delete_link) {
			tags = g_slist_remove (tags, delete_link);
			delete_link = NULL;
		}
	}
	if (first_category) {
		// Okay, now we have the string s, with the name of all
		// categories, now we have to set the value of the
		// first_category gpe_tag_pair to this new created string.
		p = first_category->data;
		g_free ((void*)p->value);
		p->value = g_strdup (g_locale_to_utf8(s->str, -1, NULL, NULL, NULL));
		g_string_free (s, TRUE);
	}
}
