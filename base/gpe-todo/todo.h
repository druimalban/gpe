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
  item_state state;

  struct todo_item *next, *prev;
};

struct todo_list 
{
  const char *title;
  int id;
  struct todo_list *next;
  struct todo_item *items;

  GtkWidget *widget;
};
