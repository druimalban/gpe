#ifndef DB_H
#define DB_H

#include <glib.h>

extern int db_open (void);
extern gboolean load_structure (void);

struct tag_value
{
  gchar *tag;
  gchar *value;
  guint oid;
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

extern gboolean db_insert_category (gchar *, guint *);
extern gboolean db_insert_attribute (gchar *, gchar *);

struct attribute
{
  guint id;
  gchar *name;
};

extern GSList *db_get_attributes (void);
extern GSList *db_get_categories (void);

#endif
