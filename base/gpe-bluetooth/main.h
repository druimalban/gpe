
typedef enum
  {
    BT_UNKNOWN,
    BT_LAP,
    BT_DUN,
    BT_NAP
  } bt_device_type;

struct bt_device
{
  gchar *name;
  guint class;
  bdaddr_t bdaddr;
  GdkPixbuf *pixbuf;
  bt_device_type type;
  guint port;
  pid_t pid;
  GtkWidget *window, *button;
};

extern gboolean radio_is_on;
