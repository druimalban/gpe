#ifndef _AERIAL_MAIN_H
#define _AERIAL_MAIN_H

#include <gtk/gtk.h>
#include "prismstumbler.h"

typedef struct
{
	psnetinfo_t net;
	usernetinfo_t netinfo;
	GdkPixbuf *pix;
	GtkTreeIter iter;
	int visible;
}
netinfo_t;

extern gboolean radio_is_on;

extern GSList *service_desc_list;

extern GtkWidget *bt_progress_dialog (gchar *text, GdkPixbuf *pixbuf);

extern GdkWindow *dock_window;
extern void schedule_message_delete (guint id, guint time);
extern void update_display (netinfo_t * ni);

#endif
