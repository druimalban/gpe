#ifndef _EXPORT_VCAL_H
#define _EXPORT_VCAL_H

#include <gpe/event-db.h>

#include <mimedir/mimedir.h>

MIMEDirVEvent *
export_event_as_vevent (Event *ev);
#endif
