#ifndef DB_H
#define DB_H

#include <glib.h>

extern int db_open (void);
extern gboolean load_structure (void);

struct tag_value
{
  gchar *tag;
  gchar *value;
};

extern struct tag_value *new_tag_value (gchar *, gchar *);
extern void free_tag_values (GSList *);
extern void update_tag_value (struct tag_value *t, gchar *value);

struct person
{
  guint id;
  gchar *name;
  GSList *data;
};

extern struct person *new_person (void);
extern gboolean commit_person (struct person *);
extern void discard_person (struct person *);
extern void db_set_data (struct person *, gchar *tag, gchar *value);
extern struct tag_value *db_find_tag (struct person *p, gchar *tag);
extern GSList *db_get_entries (void);
extern struct person *db_get_by_uid (guint uid);
extern gboolean db_delete_by_uid (guint uid);
extern void db_set_multi_data (struct person *p, gchar *tag, gchar *value);
extern void db_delete_tag (struct person *p, gchar *tag);

extern gboolean db_insert_category (gchar *, guint *);
extern GSList *db_get_entries_alpha (const gchar* alphalist);
extern gboolean db_delete_category (guint id);

struct category
{
  guint id;
  gchar *name;
};

extern GSList *db_get_categories (void);

#endif
