#include <glib.h>

struct day_popup
{
  guint year, month, day;
  GSList *events;
};

extern GtkMenu *day_popup (struct day_popup *p, gboolean show_events);
