#ifndef GPE_BATTERY_H
#define GPE_BATTERY_H

GtkWidget *Battery_Build_Objects();
void Battery_Free_Objects();
void Battery_Restore();

/* from ipaq kernel, h3600_ts.h */
#define H3600_BATT_STATUS_HIGH          0x01
#define H3600_BATT_STATUS_LOW           0x02
#define H3600_BATT_STATUS_CRITICAL      0x04
#define H3600_BATT_STATUS_CHARGING      0x08
#define H3600_BATT_STATUS_CHARGE_MAIN   0x10
#define H3600_BATT_STATUS_DEAD          0x20   /* Battery will not charge */
#define H3600_BATT_NOT_INSTALLED        0x20   /* For expansion pack batteries */
#define H3600_BATT_STATUS_FULL          0x40   /* Battery fully charged (and connected to AC) */
#define H3600_BATT_STATUS_NOBATT        0x80
#define H3600_BATT_STATUS_UNKNOWN       0xff

#endif
