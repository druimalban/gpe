#define _PATH_PROCNET_DEV	"/proc/net/dev"
#define _PATH_PROCNET_WIRELESS	"/proc/net/wireless" 

GtkWidget *Network_Build_Objects();
void Network_Save();
void Network_Free_Objects();
void Network_Restore();
