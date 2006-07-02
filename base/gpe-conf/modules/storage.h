#ifndef GPE_STORAGE_H
#define GPE_STORAGE_H

#define FS_T_FLASH 0
#define FS_T_MMC 1
#define FS_T_CF 2
#define FS_T_NFS 3
#define FS_T_UNKNOWN 4

typedef struct
{
	int total;
	int used;
	int avail;
	char *name;
	char *mountpoint;
	int type;
	GtkWidget *bar;
	GtkWidget *label, *label2, *label3, *dummy;
	int present;
}
tfs;

GtkWidget *Storage_Build_Objects();
void Storage_Free_Objects();
void Storage_Save();
void Storage_Restore();

/* helper function, may be interesting for other applets */
void toolbar_set_style (GtkWidget * bar, gint percent); 

int system_memory (void); /* used by sysinfo */

extern tfs meminfo;

#endif
