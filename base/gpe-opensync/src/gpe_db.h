#ifndef GPE_DB_H
#define GPE_DB_H

//static int fetch_uid_callback (void *arg, int argc, char **argv, char **names)
GSList * fetch_uid_list (nsqlc *db, const gchar *query, ...);

//static int fetch_callback (void *arg, int argc, char **argv, char **names);
GSList * fetch_tag_data (nsqlc *db, const gchar *query_str, guint id);

gchar * get_tag_value(GSList *tags, gchar *tag);

gboolean store_tag_data (nsqlc *db, const gchar *table, guint id, GSList *tags, gboolean delete);

void s_print_tags(GSList *tags, GString *str);

int get_new_urn (nsqlc *db);

GSList * fetch_categories (nsqlc *db);

void tags_replace_category (GSList *tags, GSList *categories);

#endif /* GPE_DB_H */
