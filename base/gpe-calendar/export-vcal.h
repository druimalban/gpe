#ifndef _EXPORT_VCAL_H
#define _EXPORT_VCAL_H

#include <gpe/event-db.h>

void vcal_do_send_bluetooth (Event *event);
void vcal_do_send_irda (Event *event);

/* Converts event EV to a string.  The caller must free the returned
   string using g_free.  */
extern char *export_event_as_string (Event *ev);

/* Converts calendar EV to a string.  The caller must free the
   returned string using g_free.  */
extern char *export_calendar_as_string (EventCalendar *ev);

/* Prompts the user with a save as dialog.  Does not consume a
   reference to EVENT.  */
extern void export_event_save_as_dialog (Event *event);

/* Prompts the user with a save as dialog.  Does not consume a
   reference to EVENT.  */
extern void export_calendar_save_as_dialog (EventCalendar *event);

void vcal_export_init (void);
gboolean export_bluetooth_available (void);
gboolean export_irda_available (void);
#endif
