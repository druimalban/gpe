#ifndef _GLOBALS_H
#define _GLOBALS_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>

char *atom_names[] = 
  {
    "WM_PROTOCOLS",
    "_NET_WM_PING",
    "WM_DELETE_WINDOW",
    "_NET_CLIENT_LIST",
    "_NET_ACTIVE_WINDOW",
    "_NET_WM_WINDOW_TYPE",
    "_NET_WM_WINDOW_TYPE_DESKTOP",
    "_NET_WM_WINDOW_TYPE_DOCK",
    "_NET_WM_WINDOW_TYPE_TOOLBAR"
  };

#define WM_PROTOCOLS 0
#define _NET_WM_PING 1
#define WM_DELETE_WINDOW 2
#define _NET_CLIENT_LIST 3
#define _NET_ACTIVE_WINDOW 4
#define _NET_WM_WINDOW_TYPE 5
#define _NET_WM_WINDOW_TYPE_DESKTOP 6
#define _NET_WM_WINDOW_TYPE_DOCK 7
#define _NET_WM_WINDOW_TYPE_TOOLBAR 8
#endif
