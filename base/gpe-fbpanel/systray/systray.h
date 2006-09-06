#ifndef _SYSTRAY_H
#define _SYSTRAY_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "eggtraymanager.h"
    
typedef struct {
    GtkWidget *mainw;
    plugin *plug;
    GtkWidget *box;
    GtkWidget *context_menu;      /* context menu for the panel */
    gboolean pack_start;
    EggTrayManager *tray_manager;
} tray;

void tray_add_widget (tray *tr, GtkWidget *widget);
void tray_save_session (tray *tr);
void tray_restore_session (tray *tr);

#ifdef __cplusplus
}
#endif

#endif /* _SYSTRAY_H */
