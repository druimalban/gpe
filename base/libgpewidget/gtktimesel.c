/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <libintl.h>
#include <gtk/gtk.h>
#include "gtktimesel.h"

#define _(x) gettext(x)

struct _GtkTimeSelClass 
{
  GtkHBoxClass parent_class;
};

static GtkHBoxClass *parent_class = NULL;

static void
gtk_time_sel_init (GtkTimeSel *sel)
{
  sel->hour_adj = gtk_adjustment_new (0, 0, 23, 1, 15, 15);
  sel->minute_adj = gtk_adjustment_new (0, 0, 59, 1, 15, 15);

  sel->hour_spin = gtk_spin_button_new (GTK_ADJUSTMENT (sel->hour_adj), 1, 0);
  sel->minute_spin = gtk_spin_button_new (GTK_ADJUSTMENT (sel->minute_adj), 1, 0);

  sel->label = gtk_label_new (":");

  gtk_box_pack_start (GTK_BOX (sel), sel->hour_spin, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (sel), sel->label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (sel), sel->minute_spin, TRUE, TRUE, 0);
}

static void
gtk_time_sel_show (GtkWidget *widget)
{
  GtkTimeSel *sel;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_TIME_SEL (widget));

  sel = GTK_TIME_SEL (widget);

  gtk_widget_show (sel->hour_spin);
  gtk_widget_show (sel->minute_spin);
  gtk_widget_show (sel->label);

  GTK_WIDGET_CLASS (parent_class)->show (widget);
}

static void
gtk_time_sel_class_init (GtkTimeSelClass * klass)
{
  GObjectClass *oclass;
  GtkWidgetClass *widget_class;

  parent_class = g_type_class_ref (gtk_hbox_get_type ());
  oclass = (GObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  widget_class->show = gtk_time_sel_show;
}

GtkType
gtk_time_sel_get_type (void)
{
  static GType time_sel_type = 0;

  if (! time_sel_type)
    {
      static const GTypeInfo time_sel_info =
      {
	sizeof (GtkTimeSelClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gtk_time_sel_class_init,
	(GClassFinalizeFunc) NULL,
	NULL /* class_data */,
	sizeof (GtkTimeSel),
	0 /* n_preallocs */,
	(GInstanceInitFunc) gtk_time_sel_init,
      };

      time_sel_type = g_type_register_static (GTK_TYPE_HBOX, "GtkTimeSel", &time_sel_info, (GTypeFlags)0);
    }
  return time_sel_type;
}

void
gtk_time_sel_get_time (GtkTimeSel *sel, guint *hour, guint *minute)
{
  if (hour)
    *hour = (guint) gtk_adjustment_get_value (GTK_ADJUSTMENT (sel->hour_adj));
  if (minute)
    *minute = (guint) gtk_adjustment_get_value (GTK_ADJUSTMENT (sel->minute_adj));
}

void
gtk_time_set_set_time (GtkTimeSel *sel, guint hour, guint minute)
{
  gtk_adjustment_set_value (GTK_ADJUSTMENT (sel->hour_adj), hour);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (sel->minute_adj), minute);
}

GtkWidget *
gtk_time_sel_new (void)
{
  return GTK_WIDGET (g_object_new (gtk_time_sel_get_type (), NULL));
}
