void bluetooth_send_file (const gchar *uri, GnomeVFSFileInfo *info);
void irda_send_file (const gchar *uri, GnomeVFSFileInfo *info);
gboolean bluetooth_available (void);
gboolean irda_available (void);
void bluetooth_init (void);



