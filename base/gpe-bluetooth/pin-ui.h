typedef struct _BluetoothPinRequest BluetoothPinRequest;
typedef struct _BluetoothPinRequestClass BluetoothPinRequestClass;

BluetoothPinRequest *bluetooth_pin_request_new (gboolean outgoing, const gchar *address, const gchar *name);
