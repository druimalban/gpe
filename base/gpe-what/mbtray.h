/* libmbdock 
 * Copyright (C) 2002 Matthew Allum
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _TRAY_H_
#define _TRAY_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>

#include "mbpixbuf.h"

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
#define ATOM_MB_PANEL_BG     7
#define ATOM_NET_WM_ICON     8
#define ATOM_NET_WM_PID      9

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
#define TRAYDBG(txt, args... ) \
        fprintf(stderr, "TRAY-DEBUG: " txt "\n", ##args )
#else
#define TRAYDBG(txt, args... ) /* nothing */
#endif

typedef void (*MBTrayBackgroundCB)( void *user_data ) ;

int  mb_tray_init(Display* dpy, Window win);

Window mb_tray_get_window(void);

void mb_tray_init_session_info(Display *d, Window win, char **argv, int argc);

void mb_tray_handle_event(Display *dpy, Window win, XEvent *an_event);

void mb_tray_send_message(Display *d, Window win, 
			  unsigned char* msg, int timeout);

void mb_tray_map_window (Display* dpy, Window win);

void
mb_tray_bg_change_cb_set(MBTrayBackgroundCB bg_changed_cb, void *user_data);

Bool mb_tray_get_bg_col(Display *dpy, XColor *xcol);

void
mb_tray_window_icon_set(Display *dpy, Window win_panel, MBPixbufImage *img);

void mb_tray_unmap_window (Display* dpy, Window win);

MBPixbufImage *mb_tray_get_bg_img(MBPixbuf *pb, Window win);


#endif
