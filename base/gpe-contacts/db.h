#ifndef DB_H
#define DB_H

#include <glib.h>

extern int db_open (void);
extern gboolean load_structure (void);

/* Well known tags */
#define TAG_PHONE_HOME		100
#define TAG_PHONE_WORK		101
#define TAG_PHONE_MOBILE	102
#define TAG_PHONE_FAX		103
#define TAG_PHONE_PAGER		104
#define TAG_PHONE_TELEX		105

#define TAG_ADDRESS_HOME	200
#define TAG_ADDRESS_WORK	201

#define TAG_INTERNET_EMAIL	300
#define TAG_INTERNET_WEB	301

#define TAG_SUMMARY		400
#define TAG_BIRTHDAY		401
#define TAG_NOTES		402
#define TAG_CATEGORIES		403

struct tag_value
{
  guint tag;
  gchar *value;
};

extern struct tag_value *new_tag_value (guint, gchar *);
extern void free_tag_values (GSList *);
extern void update_tag_value (struct tag_value *t, gchar *value);

struct person
{
  struct tag_value *data;
};

#endif
