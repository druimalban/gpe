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
