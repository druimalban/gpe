#ifndef GPE_IR_MAIN_H
#define GPE_IR_MAIN_H

#include <gtk/gtk.h>

extern gboolean radio_is_on;

extern GSList *service_desc_list;

extern GdkWindow *dock_window;
extern void schedule_message_delete (guint id, guint time);
#endif
