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
    gboolean pack_start;
    /////
    EggTrayManager *tray_manager;
    
} tray;


#ifdef __cplusplus
}
#endif

#endif /* _SYSTRAY_H */
