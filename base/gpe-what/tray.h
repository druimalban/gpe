#ifndef _TRAY_H_
#define _TRAY_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

#define ATOM_SYSTEM_TRAY 0
#define ATOM_SYSTEM_TRAY_OPCODE 1
#define ATOM_XEMBED_INFO 2
#define ATOM_XEMBED 3
#define ATOM_MANAGER 4
#define ATOM_NET_SYSTEM_TRAY_MESSAGE_DATA 5
#define ATOM_KDE_SYSTEM_TRAY 6

#define MAX_SUPPORTED_XEMBED_VERSION 1

#define XEMBED_MAPPED          (1 << 0)

/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY  0
#define XEMBED_WINDOW_ACTIVATE  1
#define XEMBED_WINDOW_DEACTIVATE  2
#define XEMBED_REQUEST_FOCUS 3
#define XEMBED_FOCUS_IN  4
#define XEMBED_FOCUS_OUT  5
#define XEMBED_FOCUS_NEXT 6
#define XEMBED_FOCUS_PREV 7
/* 8-9 were used for XEMBED_GRAB_KEY/XEMBED_UNGRAB_KEY */
#define XEMBED_MODALITY_ON 10
#define XEMBED_MODALITY_OFF 11
#define XEMBED_REGISTER_ACCELERATOR     12
#define XEMBED_UNREGISTER_ACCELERATOR   13
#define XEMBED_ACTIVATE_ACCELERATOR     14

#ifdef DEBUG
#define TRAYDBG(txt, args... ) fprintf(stderr, "TRAY-DEBUG: " txt , ##args )
#else
#define TRAYDBG(txt, args... ) /* nothing */
#endif

int  tray_init(Display* dpy, Window win);

Window tray_get_window(void);

void tray_init_session_info(Display *d, Window win, char **argv, int argc);

void tray_handle_event(Display *dpy, Window win, XEvent *an_event);

void tray_send_message(Display *d, Window win, unsigned char* msg);

void tray_map_window (Display* dpy, Window win);

/* probably wont work */
void tray_unmap_window (Display* dpy, Window win);

#endif
