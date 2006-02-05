#include <gtk/gtk.h>
#include <bluetooth/sdp.h>

struct bt_device
{
  gchar *name;
  guint class;
  bdaddr_t bdaddr;
  GdkPixbuf *pixbuf;
  gboolean sdp;
  GSList *services;
};

struct bt_service
{
  struct bt_service_desc *desc;
};

struct bt_service_desc
{
  uuid_t uuid;

  struct bt_service * (*scan)(sdp_record_t *rec, struct bt_device *bd);
  void (*popup_menu)(struct bt_service *, GtkWidget *);
};

extern gboolean radio_is_on;

extern GSList *service_desc_list;

extern bdaddr_t src_addr;

extern GtkWidget *bt_progress_dialog (gchar *text, GdkPixbuf *pixbuf);
extern void bt_progress_dialog_update (GtkWidget *w, gchar *new_text);

extern GdkWindow *dock_window;
extern void schedule_message_delete (guint id, guint time);

extern const gchar *icon_name_for_class (int class);

extern sdp_session_t *sdp_session;
