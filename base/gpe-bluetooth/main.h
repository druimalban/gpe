#include <gdk-pixbuf/gdk-pixbuf.h>

typedef enum
  {
    BT_UNKNOWN,
    BT_LAP,
    BT_DUN,
    BT_NAP
  } bt_service_type;

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
  struct bt_device *dev;
  bt_service_type type;
};

struct bt_service_desc
{
  uuid_t uuid;

  struct bt_service * (*scan)(sdp_record_t *rec);
};

extern gboolean radio_is_on;

extern GSList *service_desc_list;
