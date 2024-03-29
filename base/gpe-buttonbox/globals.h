#ifndef _GLOBALS_H
#define _GLOBALS_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#define _NET_WM_WINDOW_TYPE_DOCK 0
#define _NET_WM_WINDOW_TYPE 1
#define _NET_WM_STATE_HIDDEN 2
#define _NET_WM_SKIP_PAGER 3
#define _NET_WM_WINDOW_TYPE_DESKTOP 4
#define _PSION_DESKTOP_SWIZZLE 5
#define _NET_ACTIVE_WINDOW 6
#define _NET_SHOWING_DESKTOP 7
#define _NET_SYSTEM_TRAY_OPCODE 8
#define _NET_SYSTEM_TRAY_MESSAGE_DATA 9
#define MANAGER 10
#define _NET_SYSTEM_TRAY_Sn 11

#define NR_SLOTS_DEFAULT     4
#define SLOT_WIDTH_DEFAULT  64
#define SLOT_HEIGHT_DEFAULT 48
#define ICON_SIZE_DEFAULT   24
#define MYFILES_DEFAULT     FALSE
#define LABLES_DEFAULT      TRUE

extern Atom atoms[];
extern Display *dpy;
extern GtkWidget *window;

extern gboolean system_tray_init (Display *dpy, Window window);
#endif
