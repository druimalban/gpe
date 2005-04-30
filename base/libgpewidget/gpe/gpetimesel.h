/*
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef GPE_TIME_SEL_H
#define GPE_TIME_SEL_H

#include <gtk/gtk.h>
#include <glib-object.h>

#define GPE_TYPE_TIME_SEL                  (gpe_time_sel_get_type ())
#define GPE_TIME_SEL(obj)                  G_TYPE_CHECK_INSTANCE_CAST ((obj), gpe_time_sel_get_type(), GpeTimeSel)
#define GPE_IS_TIME_SEL(obj)	G_TYPE_CHECK_INSTANCE_TYPE ((obj), gpe_time_sel_get_type ())

struct _GpeTimeSel
{
  GtkHBox hbox;

  GtkObject *hour_adj, *minute_adj;
  GtkWidget *hour_spin, *minute_spin;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *popup;
};

/**
 * GpeTimeSel:
 *
 * Time selection widget container. 
 */
typedef struct _GpeTimeSel	   GpeTimeSel;
typedef struct _GpeTimeSelClass    GpeTimeSelClass;

GtkType		gpe_time_sel_get_type (void);

GtkWidget      *gpe_time_sel_new ();

void		gpe_time_sel_get_time (GpeTimeSel *sel, guint *hour, guint *minute);
void		gpe_time_sel_set_time (GpeTimeSel *sel, guint hour, guint minute);

#endif
