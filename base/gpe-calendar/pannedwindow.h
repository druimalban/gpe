/* pannedwindow.h - Panned Window Interface.
   Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#ifndef PANNED_WINDOW_H
#define PANNED_WINDOW_H

#include <gtk/gtkeventbox.h>

G_BEGIN_DECLS

#define TYPE_PANNED_WINDOW (panned_window_get_type ())
#define PANNED_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PANNED_WINDOW, PannedWindow))
#define PANNED_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_PANNED_WINDOW, \
                            PannedWindowClass))
#define GTK_IS_PANNED_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PANNED_WINDOW))
#define GTK_IS_PANNED_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_PANNED_WINDOW))
#define PANNED_WINDOW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_PANNED_WINDOW, \
                              PannedWindowClass))

typedef struct _PannedWindow       PannedWindow;
typedef struct _PannedWindowClass  PannedWindowClass;

struct _PannedWindow
{
  GtkEventBox event_box;
};

enum PannedWindowEdge
  {
    PANNED_NORTH = 1 << 0,
    PANNED_EAST = 1 << 1,
    PANNED_SOUTH = 1 << 2,
    PANNED_WEST = 1 << 3,

    PANNED_NORTH_EAST = PANNED_NORTH | PANNED_EAST,
    PANNED_NORTH_WEST = PANNED_NORTH | PANNED_WEST,
    PANNED_SOUTH_EAST = PANNED_SOUTH | PANNED_EAST,
    PANNED_SOUTH_WEST = PANNED_SOUTH | PANNED_WEST
  };

struct _PannedWindowClass
{
  GtkEventBoxClass parent_class;
  
  /** edge_flip
   */
  void (*edge_flip) (PannedWindow *panned_window,
		     enum PannedWindowEdge edge);

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};


extern GType panned_window_get_type (void) G_GNUC_CONST;

/* A panned window contains a GtkScrolledWindow.  You can access it
   using:

     GTK_BIN (panned_window)->child

   Place your widget in the scrolled window:

     gtk_container_add (GTK_BIN (panned_window)->child, widget)

   If your widget intercepts the button-press-event signal and returns
   TRUE, i.e. prevents further processing of the event, then the
   widget will not be panned.  This is useful for making on certain
   areas of the widget sensitive to panning.
 */
extern GtkWidget *panned_window_new (void);

/* When the mouse button is release, this will return if the widget
   was being panned.  If the button release event is not intercepted
   and handled (e.g. by returing TRUE) then panning will be
   immediately disabled.  Otherwise, a delay will be introduced until
   the mouse motion is next processed.  */
extern gboolean panned_window_is_panning (PannedWindow *pw);

G_END_DECLS

#endif /* PANNED_WINDOW_H */
