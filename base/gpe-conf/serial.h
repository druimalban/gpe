#ifndef GPE_SERIAL_H
#define GPE_SERIAL_H

typedef enum {
	SA_CONSOLE,
	SA_GPSD,
	SA_NONE
}
t_serial_assignment;

void assign_serial_port(t_serial_assignment type);

GtkWidget *Serial_Build_Objects();
void Serial_Free_Objects();
void Serial_Save();
void Serial_Restore();

#endif
