#include <glib.h>

struct day_popup
{
  guint year, month, day;
  GSList *events;
};

extern GtkWidget *day_popup(GtkWidget *parent, struct day_popup *p, gboolean show_items);
