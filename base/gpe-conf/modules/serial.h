#ifndef GPE_SERIAL_H
#define GPE_SERIAL_H

typedef enum {
	SA_CONSOLE,
	SA_GPSD,
	SA_NONE
}
t_serial_assignment;

void assign_serial_port(t_serial_assignment type);
void update_gpsd_settings (char *baud, int emate, char* port);
const gchar *get_first_serial_port (void);

GtkWidget *Serial_Build_Objects();
void Serial_Free_Objects();
void Serial_Save();
void Serial_Restore();

extern gchar *portlist[][2];
extern gint num_ports;

#endif
