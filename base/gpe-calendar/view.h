/* view.c - View widget interface.
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

#ifndef VIEW_H
#define VIEW_H

#include <gtk/gtk.h>
#include <time.h>
 
struct _GtkView
{
  GtkVBox widget;
  time_t time;
};
typedef struct _GtkView GtkView;

typedef struct
{
  GtkVBoxClass vbox_class;
  GObjectClass parent_class;

  /* Virtual methods.  */
  void (*set_time) (GtkView *view, time_t time);
  void (*reload_events) (GtkView *view);

  /* Signals.  */
  guint time_changed_signal;
  void (*time_changed) (GtkView *view, gulong time);
} GtkViewClass;

#define GTK_VIEW(obj) \
  GTK_CHECK_CAST (obj, gtk_view_get_type (), struct _GtkView)
#define GTK_VIEW_CLASS(klass) \
  GTK_CHECK_CLASS_CAST (klass, gtk_view_get_type (), ViewClass)
#define GTK_IS_VIEW(obj) GTK_CHECK_TYPE (obj, gtk_view_get_type ())

/* Return GType of a view.  */
extern GType gtk_view_get_type (void);

/* Return the time view VIEW is showing.  */
extern time_t gtk_view_get_time (GtkView *view);

/* Set the time view VIEW is showing to TIME.  */
extern void gtk_view_set_time (GtkView *view, time_t time);

/* Return the date component of the time view VIEW is showing in
   *DATE.  */
extern void gtk_view_get_date (GtkView *view, GDate *date);

/* Set the date component of the time view VIEW is showing to
   DATE.  */
extern void gtk_view_set_date (GtkView *view, GDate *date);

/* Cause view VIEW to reread the event db.  */
extern void gtk_view_reload_events (GtkView *view);

#endif
