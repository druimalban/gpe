#ifndef DB_H
#define DB_H

#include <glib.h>

#include <gpe/pim-categories.h>

#define CONFIG_PANEL 0
#define CONFIG_LIST  1

extern int db_open (gboolean open_vcard);
extern gboolean load_structure (gchar* default_structure);

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
  gchar *family_name;
  gchar *company;
  GSList *data;
};

extern struct person *new_person (void);
extern gboolean commit_person (struct person *);
extern void discard_person (struct person *);
extern void db_set_data (struct person *, gchar *tag, gchar *value);
extern struct tag_value *db_find_tag (struct person *p, gchar *tag);
extern GSList *db_get_entries (void);
extern GSList *db_get_entries_finddlg (const gchar *str, const gchar *cat);
extern GSList *db_get_entries_list (const gchar *name, const gchar *cat);

extern struct person *db_get_by_uid (guint uid);
extern gboolean db_delete_by_uid (guint uid);
extern void db_set_multi_data (struct person *p, gchar *tag, gchar *value);
extern void db_delete_tag (struct person *p, gchar *tag);

extern gint db_get_tag_list (gchar ***list);
extern gint db_get_config_values (gint group, gchar ***list);
extern void db_add_config_values (gint group, gchar *identifier, gchar *value);
extern void db_delete_config_values (gint group, gchar *identifier);
void db_update_config_values (gint group, gchar * identifier, gchar * value);
extern gchar* db_get_config_tag (gint group, const gchar *tagname);
extern void db_free_result(char** table);

extern gint sort_entries (struct person * a, struct person * b);

gchar *db_compress (void);
gint db_size (void);

#endif
