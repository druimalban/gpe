/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef GTK_DATE_COMBO_H
#define GTK_DATE_COMBO_H

#include <gtk/gtk.h>

#define GTK_TYPE_DATE_COMBO                  (gtk_date_combo_get_type ())
#define GTK_DATE_COMBO(obj)                  (GTK_CHECK_CAST ((obj), GTK_TYPE_DATE_COMBO, GtkDateCombo))
#define GTK_DATE_COMBO_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_DATE_COMBO, GtkDateComboClass))
#define GTK_IS_DATE_COMBO(obj)               (GTK_CHECK_TYPE ((obj), GTK_TYPE_DATE_COMBO))
#define GTK_IS_DATE_COMBO_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_DATE_COMBO))

struct _GtkDateCombo
{
  GtkHBox hbox;
  GtkWidget *cal;
  GtkWidget *calw;
  GtkWidget *entry;
  GtkWidget *button;
  gboolean cal_open;
  guint year, month, day;
  gboolean set;
};

struct _GtkDateComboClass 
{
  GtkHBoxClass parent_class;
};

typedef struct _GtkDateCombo	   GtkDateCombo;
typedef struct _GtkDateComboClass  GtkDateComboClass;

GtkType		gtk_date_combo_get_type	   (void);

GtkWidget      *gtk_date_combo_new (void);

void            gtk_date_combo_set_date (GtkDateCombo *,
					 guint year, guint month, guint day);

void		gtk_date_combo_clear (GtkDateCombo *dp);

void		gtk_date_combo_week_starts_monday (GtkDateCombo *, gboolean);

#endif
