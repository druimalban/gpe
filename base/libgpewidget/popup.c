/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
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

#include <X11/Xlib.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "popup.h"

int
gpe_get_win_position (Display *dpy, Window win, int *x, int *y)
{
  Window root, parent, *children ;
  unsigned int nchildren ;
  unsigned int udumm ;
  
  *x = 0 ;
  *y = 0 ;
  
  while (XQueryTree (dpy, win, &root, &parent, &children, &nchildren))
    {
      int w_x, w_y ;
      unsigned int border_w ;
      if (children) 
        XFree (children);

      if (!XGetGeometry( dpy, win, &root, &w_x, &w_y, &udumm, &udumm, &border_w, &udumm ))
        break ;

      (*x)+=w_x+(int)border_w ;
      (*y)+=w_y+(int)border_w ;
      
      if (parent == root)
        return 1;

      win = parent ;
    }

  *x = 0 ;
  *y = 0 ;
  return 0 ;
}

void
gpe_popup_menu_position (GtkMenu *menu, gint *xp, gint *yp, gboolean *push, gpointer p)
{
  GtkWidget *w = GTK_WIDGET (p);
  int x, y;
  
  gpe_get_win_position (GDK_WINDOW_XDISPLAY (w->window), GDK_WINDOW_XWINDOW (w->window), &x, &y);

  *xp = x;
  if (GTK_WIDGET_NO_WINDOW (w))
    x += w->allocation.x;
  *yp = y - GTK_WIDGET (menu)->requisition.height;

  if (*yp < 0)
    *yp = y + w->allocation.height;
}
