#include <sys/time.h>
#include <gtk/gtk.h>

typedef enum
{
  NOT_STARTED,
  IN_PROGRESS,
  COMPLETED
} item_state;

struct todo_item
{
  time_t time;
  const char *what;
  const char *summary;
  item_state state;
  time_t due;
};

struct todo_list 
{
  const char *title;
  int id;
  GList *items;

  GtkWidget *widget;
};

extern GtkWidget *edit_todo(struct todo_list *list, struct todo_item *item);
extern GSList *lists;
extern struct todo_list *new_list (int id, const char *title);
extern GtkWidget *the_notebook;
extern void add_new_event(struct todo_list *list, time_t t, 
			  const char *what, item_state state, 
			  const char *summary);
