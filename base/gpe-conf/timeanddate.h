#ifndef _GPE_CFG_TIMEANDDATE_H
#define _GPE_CFG_TIMEANDDATE_H

GtkWidget *Time_Build_Objects();
void Time_Free_Objects();
void Time_Save();
void Time_Restore();

void set_timezone (gchar *zone);

#endif
