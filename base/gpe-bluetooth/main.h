
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
  bt_service_type type;
  guint port;
  pid_t pid;
};

extern gboolean radio_is_on;
