#ifndef GPE_STORAGE_H
#define GPE_STORAGE_H

GtkWidget *Storage_Build_Objects();
void Storage_Free_Objects();
void Storage_Save();
void Storage_Restore();

/* helper function, may be interesting for other applets */
void toolbar_set_style (GtkWidget * bar, gint percent); 

#endif
