#include <glib.h>

struct day_popup
{
  guint year, month, day;
  GSList *events;
};

extern GtkObject *day_popup(GtkWidget *parent, struct day_popup *p);
