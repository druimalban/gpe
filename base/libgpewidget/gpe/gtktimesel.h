/*
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef GTK_TIME_SEL_H
#define GTK_TIME_SEL_H

#include <gtk/gtk.h>
#include <glib-object.h>

#define GTK_TYPE_TIME_SEL                  (gtk_time_sel_get_type ())
#define GTK_TIME_SEL(obj)                  G_TYPE_CHECK_INSTANCE_CAST ((obj), gtk_time_sel_get_type(), GtkTimeSel)
#define GTK_TIME_SEL_CONST(obj)	G_TYPE_CHECK_INSTANCE_CAST ((obj), gtk_time_sel_get_type(), GtkTime_Sel const)
#define GTK_TIME_SEL_CLASS(klass)	G_TYPE_CHECK_CLASS_CAST ((klass), gtk_time_sel_get_type(), GtkTime_SelClass)
#define GTK_IS_TIME_SEL(obj)	G_TYPE_CHECK_INSTANCE_TYPE ((obj), gtk_time_sel_get_type ())

#define GTK_TIME_SEL_GET_CLASS(obj)	G_TYPE_INSTANCE_GET_CLASS ((obj), gtk_time_sel_get_type(), GtkTime_SelClass)

struct _GtkTimeSel
{
  GtkHBox hbox;

  GtkObject *hour_adj, *minute_adj;
  GtkWidget *hour_spin, *minute_spin;
  GtkWidget *label;
};

typedef struct _GtkTimeSel	   GtkTimeSel;
typedef struct _GtkTimeSelClass    GtkTimeSelClass;

GtkType		gtk_time_sel_get_type (void);

GtkWidget      *gtk_time_sel_new ();

void		gtk_time_sel_get_time (GtkTimeSel *sel, guint *hour, guint *minute);
void		gtk_time_sel_set_time (GtkTimeSel *sel, guint hour, guint minute);

#endif
