#include <gtk/gtk.h>
#include "prismstumbler.h"

typedef struct
{
	char bssid[20];
	char ssid[33];
	int mode; 		// o = managed, 1 = ad-hoc
	int wep;
	int dhcp;
	int channel;
	unsigned char ip[4];
	unsigned char netmask[4];
	unsigned char gateway[4];
	char wep_key[48];
	int inrange;	
}
usernetinfo_t;

typedef struct
{
	psnetinfo_t net;
	usernetinfo_t netinfo;
	GdkPixbuf *pix;
}
netinfo_t;

extern gboolean radio_is_on;

extern GSList *service_desc_list;

extern GtkWidget *bt_progress_dialog (gchar *text, GdkPixbuf *pixbuf);

extern GdkWindow *dock_window;
extern void schedule_message_delete (guint id, guint time);
