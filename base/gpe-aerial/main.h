#include <gtk/gtk.h>
#include "prismstumbler.h"

typedef struct
{
	psnetinfo_t net;
	GdkPixbuf *pix;
}
netinfo_t;

extern gboolean radio_is_on;

extern GSList *service_desc_list;

extern GtkWidget *bt_progress_dialog (gchar *text, GdkPixbuf *pixbuf);

extern GdkWindow *dock_window;
extern void schedule_message_delete (guint id, guint time);
