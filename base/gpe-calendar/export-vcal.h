#ifndef _EXPORT_VCAL_H
#define _EXPORT_VCAL_H

#include <gpe/event-db.h>

void vcal_do_send_bluetooth (event_t event);
void vcal_do_send_irda (event_t event);
void vcal_do_save (event_t event);
void vcal_export_init (void);
gboolean export_bluetooth_available (void);
gboolean export_irda_available (void);
gboolean save_to_file(event_t event, const gchar *filename);
#endif
