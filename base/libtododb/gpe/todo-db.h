#ifndef TODODB_H
#define TODODB_H

#include <glib.h>

typedef enum
{
  NOT_STARTED,
  IN_PROGRESS,
  COMPLETED,
  ABANDONED
} item_state;

struct todo_item
{
  int id, pos;
  time_t time;
  const char *what;
  const char *summary;
  item_state state;
  gboolean was_complete;
  GSList *categories;
  guint priority;
};

#define PRIORITY_HIGH		9
#define PRIORITY_STANDARD	5
#define PRIORITY_LOW		0

extern struct todo_item *todo_db_new_item (void);
extern gboolean todo_db_push_item (struct todo_item *i);
extern void todo_db_delete_item (struct todo_item *i);
extern void todo_db_destroy_item (struct todo_item *i);

extern GSList *todo_db_get_items_list(void);

extern int todo_db_start (void);
extern void todo_db_stop (void);
extern int todo_db_refresh (void);

#endif
