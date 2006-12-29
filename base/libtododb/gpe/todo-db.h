#ifndef TODODB_H
#define TODODB_H

#include <glib.h>

/** 
 * item_state:
 * @NOT_STARTED: Indicates a task which isn't started.
 * @IN_PROGRESS: Task currently in progress.
 * @COMPLETED: Task is marked as completed.
 * @ABANDONED: Indicates an abandoned task.
 *
 * Type to describe the progress status of a todo item.
 */
typedef enum
{
  NOT_STARTED,
  IN_PROGRESS,
  COMPLETED,
  ABANDONED
} item_state;


/** 
 * todo_item:
 * @id: Unique id.
 * @pos: Position marker.
 * @time: Timestamp holding due date.
 * @what: Item title.
 * @summary: Item description.
 * @todoid: ID for vtodo ex-/importation.
 * @state: Item status (see #item_state).
 * @was_complete:
 * @categories: List of categories the item belongs to.
 * @priority: Priority information.
 *
 * Data type describing a todo item including description and current status.
 */
struct todo_item
{
  int id, pos;
  time_t time;
  const char *what;
  const char *summary;
  const char *todoid;
  item_state state;
  gboolean was_complete;
  GSList *categories;
  guint priority;
};

/**
 * PRIORITY_HIGH: 
 *
 * Indicates an item with high priority.
 */
/**
 * PRIORITY_STANDARD: 
 *
 * Indicates an item with default priority.
 */
/**
 * PRIORITY_LOW: 
 *
 * Indicates a low priority item.
 */
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

/** Find the item with uid UID returning it or returning NULL if none
    is found.  */
extern struct todo_item *todo_db_find_item_by_id (guint uid);

#endif
