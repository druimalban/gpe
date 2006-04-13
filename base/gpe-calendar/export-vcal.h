#ifndef _EXPORT_VCAL_H
#define _EXPORT_VCAL_H

#include <gpe/event-db.h>

void vcal_do_send_bluetooth (Event *event);
void vcal_do_send_irda (Event *event);
void vcal_do_save (Event *event);
void vcal_export_init (void);
gboolean export_bluetooth_available (void);
gboolean export_irda_available (void);
gboolean save_to_file(Event *event, const gchar *filename);
#endif
