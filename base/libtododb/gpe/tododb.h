#ifndef TODODB_H
#define TODODB_H

#include <glib.h>

typedef enum
{
  NOT_STARTED,
  IN_PROGRESS,
  COMPLETED
} item_state;

struct todo_db_item
{
  int id, pos;
  time_t time;
  const char *what;
  const char *summary;
  item_state state;
  gboolean was_complete;
  GSList *categories;
};

struct todo_db_category
{
  const char *title;
  int id;
};

extern GSList *todo_db_categories, *todo_db_items;

extern struct todo_db_item *todo_db_new_item (void);
extern gboolean todo_db_push_item (struct todo_db_item *i);
extern void todo_db_delete_item (struct todo_db_item *i);
extern struct todo_db_category *todo_db_new_category (const char *title);
extern void todo_db_del_category (struct todo_db_category *);

extern int todo_db_start (void);

#endif
