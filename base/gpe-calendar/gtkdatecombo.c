/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <time.h>
#include <gtk/gtk.h>
#include "gtkdatecombo.h"

static void
update_text(GtkDateCombo *combo)
{
  struct tm tm;
  char buf[256];

  memset(&tm, 0, sizeof(tm));
  tm.tm_year = combo->year - 1900;
  tm.tm_mon = combo->month;
  tm.tm_mday = combo->day;
  strftime (buf, sizeof(buf), "%x", &tm);
  gtk_entry_set_text (GTK_ENTRY (combo->entry), buf);
}

static void
click_calendar(GtkWidget *widget,
	       GtkDateCombo *combo)
{
  gtk_calendar_get_date (GTK_CALENDAR (widget), &combo->year, 
			 &combo->month, &combo->day);

  update_text (combo);

  gtk_widget_hide (widget);
}

static void
drop_calendar(GtkWidget *widget,
	      GtkDateCombo *dp)
{
  if (dp->cal_open)
    {
      gtk_widget_hide (dp->calw);
      dp->cal_open = FALSE;
    }
  else
    {
      GtkRequisition requisition;
      gint x, y;
      gint screen_width;
      gint screen_height;

      gtk_calendar_select_month (GTK_CALENDAR (dp->cal), dp->month, dp->year);
      gtk_calendar_select_day (GTK_CALENDAR (dp->cal), dp->day);
 
      gdk_window_get_pointer (NULL, &x, &y, NULL);
      gtk_widget_size_request (dp->cal, &requisition);
      
      screen_width = gdk_screen_width ();
      screen_height = gdk_screen_height ();
      
      x = CLAMP (x - 2, 0, MAX (0, screen_width - requisition.width));
      y = CLAMP (y + 4, 0, MAX (0, screen_height - requisition.height));
      
      gtk_widget_set_uposition (dp->calw, MAX (x, 0), MAX (y, 0));
      
      gtk_widget_show_all (dp->calw);
      dp->cal_open = TRUE;
    }
}

static void
gtk_date_combo_init (GtkDateCombo *combo)
{
  time_t t;
  struct tm tm;
  
  time (&t);
  localtime_r (&t, &tm);

  combo->year = tm.tm_year + 1900;
  combo->month = tm.tm_mon;
  combo->day = tm.tm_mday;

  combo->button = gtk_button_new ();
  combo->entry = gtk_entry_new ();

  update_text (combo);

  gtk_box_pack_start (GTK_BOX (combo), combo->entry, TRUE, TRUE, 5);
  gtk_box_pack_start (GTK_BOX (combo), combo->button, FALSE, FALSE, 0);

  gtk_entry_set_editable (GTK_ENTRY (combo->entry), FALSE);

  combo->cal = gtk_calendar_new ();
  combo->calw = gtk_window_new (GTK_WINDOW_POPUP);
  combo->cal_open = FALSE;
  gtk_widget_ref (combo->calw);
  gtk_container_add (GTK_CONTAINER (combo->calw), combo->cal);

  gtk_window_set_policy (GTK_WINDOW (combo->calw), FALSE, FALSE, TRUE);
      
  gtk_container_add (GTK_CONTAINER (combo->button), 
		     gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT));

  gtk_signal_connect (GTK_OBJECT (combo->button), "clicked",
		      GTK_SIGNAL_FUNC (drop_calendar), combo);

  gtk_signal_connect (GTK_OBJECT (combo->cal), "day-selected-double-click",
		      GTK_SIGNAL_FUNC (click_calendar), combo);
}

static GtkHBoxClass *parent_class = NULL;

static void
gtk_date_combo_destroy (GtkObject * combo)
{
  gtk_widget_destroy (GTK_DATE_COMBO (combo)->calw);
  gtk_widget_unref (GTK_DATE_COMBO (combo)->calw);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (combo);
}

static void
gtk_date_combo_size_allocate (GtkWidget     *widget,
			      GtkAllocation *allocation)
{
  GtkDateCombo *combo;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_DATE_COMBO (widget));
  g_return_if_fail (allocation != NULL);

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);
  
  combo = GTK_DATE_COMBO (widget);

  if (combo->entry->allocation.height > combo->entry->requisition.height)
    {
      GtkAllocation button_allocation;

      button_allocation = combo->button->allocation;
      button_allocation.height = combo->entry->requisition.height;
      button_allocation.y = combo->entry->allocation.y + 
	(combo->entry->allocation.height - combo->entry->requisition.height) 
	/ 2;
      gtk_widget_size_allocate (combo->button, &button_allocation);
    }
}

static void
gtk_date_combo_class_init (GtkDateComboClass * klass)
{
  GtkObjectClass *oclass;
  GtkWidgetClass *widget_class;

  parent_class = gtk_type_class (gtk_hbox_get_type ());
  oclass = (GtkObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  oclass->destroy = gtk_date_combo_destroy;
  
  widget_class->size_allocate = gtk_date_combo_size_allocate;
}

guint
gtk_date_combo_get_type (void)
{
  static guint date_combo_type = 0;

  if (! date_combo_type)
    {
      static const GtkTypeInfo date_combo_info =
      {
	"GtkDateCombo",
	sizeof (GtkDateCombo),
	sizeof (GtkDateComboClass),
	(GtkClassInitFunc) gtk_date_combo_class_init,
	(GtkObjectInitFunc) gtk_date_combo_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      date_combo_type = gtk_type_unique (gtk_hbox_get_type (), 
					 &date_combo_info);
    }
  return date_combo_type;
}

GtkWidget *
gtk_date_combo_new (void)
{
  return GTK_WIDGET (gtk_type_new (gtk_date_combo_get_type ()));
}

void
gtk_date_combo_set_date (GtkDateCombo *dp, guint year, guint month, guint day)
{
  dp->year = year;
  dp->month = month;
  dp->day = day;
  update_text (dp);
}
