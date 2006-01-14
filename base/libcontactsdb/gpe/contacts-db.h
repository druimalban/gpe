#ifndef DB_H
#define DB_H

#include <glib.h>

#include <gpe/pim-categories.h>

#define CONFIG_PANEL 0
#define CONFIG_LIST  1

extern int db_open (gboolean open_vcard);
extern int contacts_db_open (gboolean open_vcard);
extern int contacts_db_close (void);

struct contacts_tag_value
{
  gchar *tag;
  gchar *value;
};

extern struct contacts_tag_value *contacts_new_tag_value (gchar *, gchar *);
extern void contacts_free_tag_values (GSList *);
extern void contacts_update_tag_value (struct contacts_tag_value *t, gchar *value);

struct contacts_person
{
  guint id;
  gchar *name;
  gchar *family_name;
  gchar *company;
  GSList *data;
};

extern struct contacts_person *contacts_new_person (void);
extern gboolean contacts_commit_person (struct contacts_person *);
extern void contacts_discard_person (struct contacts_person *);
extern void contacts_db_set_data (struct contacts_person *, gchar *tag, gchar *value);
extern struct contacts_tag_value *contacts_db_find_tag (struct contacts_person *p, gchar *tag);
extern GSList *contacts_db_get_entries (void);
extern GSList *contacts_db_get_entries_finddlg (const gchar *str, const gchar *cat);
extern GSList *contacts_db_get_entries_list (const gchar *name, const gchar *cat);
extern GSList *contacts_db_get_entries_list_filtered (const gchar* str, const gchar *filter, const gchar *cat);

extern struct contacts_person *contacts_db_get_by_uid (guint uid);
extern gboolean contacts_db_delete_by_uid (guint uid);
extern void contacts_db_set_multi_data (struct contacts_person *p, gchar *tag, gchar *value);
extern void contacts_db_delete_tag (struct contacts_person *p, gchar *tag);

extern gint contacts_db_get_tag_list (gchar ***list);
extern gint contacts_db_get_config_values (gint group, gchar ***list);
extern void contacts_db_add_config_values (gint group, gchar *identifier, gchar *value);
extern void contacts_db_delete_config_values (gint group, gchar *identifier);
void contacts_db_update_config_values (gint group, gchar * identifier, gchar * value);
extern gchar* contacts_db_get_config_tag (gint group, const gchar *tagname);
extern void contacts_db_free_result(char** table);

extern gint contacts_sort_entries (struct contacts_person * a, struct contacts_person * b);

gchar *contacts_db_compress (void);
gint contacts_db_size (void);

#endif
