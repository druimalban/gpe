/*
 * Copyright (C) 2001, 2002, 2003, 2006 Philip Blundell <philb@gnu.org>
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

#include <time.h>
#include <string.h>

#include <gtk/gtk.h>
#if defined(IS_HILDON) && HILDON_VER > 0
#include <hildon/hildon-calendar.h>
#endif

#include "stylus.h"
#include "pixmaps.h"
#include "gtkdatecombo.h"

static guint my_signals[1];

static void
gtk_date_combo_emit_changed (GtkDateCombo *c)
{
  g_signal_emit (G_OBJECT (c), my_signals[0], 0);
}

static void
popdown_calendar (GtkDateCombo *combo)
{
  gtk_grab_remove (combo->cal);
  gtk_widget_hide (combo->calw);
  gdk_display_pointer_ungrab (gtk_widget_get_display (GTK_WIDGET (combo->cal)),
			      gtk_get_current_event_time ());
  gdk_display_keyboard_ungrab (gtk_widget_get_display (GTK_WIDGET (combo->cal)),
			       gtk_get_current_event_time ());
  combo->cal_open = FALSE;  
}

static gint
gtk_date_combo_button_press (GtkWidget * widget, GdkEvent * event, GtkDateCombo * combo)
{
  GtkWidget *child;

  child = gtk_get_event_widget (event);

  /* We don't ask for button press events on the grab widget, so
   *  if an event is reported directly to the grab widget, it must
   *  be on a window outside the application (and thus we remove
   *  the popup window). Otherwise, we check if the widget is a child
   *  of the grab widget, and only remove the popup window if it
   *  is not.
   */
  if (child != widget)
    {
      while (child)
	{
	  if (child == widget)
	    return FALSE;

	  child = child->parent;
	}
    }
  
  popdown_calendar (combo);
  
  return TRUE;
}

static gboolean
popup_grab_on_window (GdkWindow *window,
		      guint32    activate_time)
{
  if ((gdk_pointer_grab (window, TRUE,
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK,
			 NULL, NULL, activate_time) == 0))
    {
      if (gdk_keyboard_grab (window, TRUE, activate_time) == 0)
	return TRUE;
      else
	{
	  gdk_display_pointer_ungrab (gdk_drawable_get_display (window),
				      activate_time);
	  return FALSE;
	}
    }

  return FALSE;
}

static void
update_text (GtkDateCombo *combo)
{
  struct tm tm;
  char buf[256];

  memset(&tm, 0, sizeof(tm));
  tm.tm_year = combo->year - 1900;
  tm.tm_mon = combo->month;
  tm.tm_mday = combo->day;
  
  if (combo->ignore_year)
    {
      strftime (buf, sizeof (buf), "%D", &tm);
      strrchr (buf,'/')[0] = 0;
    }
  else
    {
      strftime (buf, sizeof (buf), "%x", &tm);
    }
  
  gtk_entry_set_text (GTK_ENTRY (combo->entry), buf);
}

static void
click_calendar (GtkWidget *widget, GtkDateCombo *combo)
{
#if defined(IS_HILDON) && HILDON_VER > 0
  hildon_calendar_get_date (HILDON_CALENDAR (widget), &combo->year, 
			 &combo->month, &combo->day);
#else
  gtk_calendar_get_date (GTK_CALENDAR (widget), &combo->year, 
			 &combo->month, &combo->day);
#endif
  combo->set = TRUE;
  update_text (combo);
  popdown_calendar (combo);
  gtk_date_combo_emit_changed (combo);
}

static void
drop_calendar (GtkWidget *widget, GtkDateCombo *dp)
{
  if (dp->cal_open)
    {
      popdown_calendar (dp);
    }
  else
    {
      GtkRequisition requisition;
      gint x, y;
      gint screen_width;
      gint screen_height;

#if defined(IS_HILDON) && HILDON_VER > 0
      hildon_calendar_select_month (HILDON_CALENDAR (dp->cal), dp->month, dp->year);
      hildon_calendar_select_day (HILDON_CALENDAR (dp->cal), dp->day);
#else
      gtk_calendar_select_month (GTK_CALENDAR (dp->cal), dp->month, dp->year);
      gtk_calendar_select_day (GTK_CALENDAR (dp->cal), dp->day);
#endif
 
      gdk_window_get_pointer (NULL, &x, &y, NULL);
      gtk_widget_size_request (dp->cal, &requisition);
      
      screen_width = gdk_screen_width ();
      screen_height = gdk_screen_height ();
      
      x = CLAMP (x - 2, 0, MAX (0, screen_width - requisition.width));
      y = CLAMP (y + 4, 0, MAX (0, screen_height - requisition.height));
      
      gtk_widget_set_uposition (dp->calw, MAX (x, 0), MAX (y, 0));
      
      gtk_widget_show (dp->calw);
      dp->cal_open = TRUE;
      
      gtk_widget_grab_focus (dp->cal);
      popup_grab_on_window (dp->calw->window, gtk_get_current_event_time ());

      gtk_grab_add (dp->cal);
    }
}

static gboolean 
verify_date (GtkWidget *entry, GdkEventFocus *event, GtkDateCombo *cb)
{
  const gchar *text = gtk_entry_get_text (GTK_ENTRY (entry));
  struct tm time;
  char *ret;
  
  if (!strlen(text)) /* allow empty text */
    {
      cb->set = FALSE;
      return FALSE;
    }
  ret = strptime (text, "%x", &time);
  if (ret)
    {
      gtk_date_combo_set_date (cb, time.tm_year+1900, time.tm_mon, time.tm_mday);
      gtk_date_combo_emit_changed (cb);
      cb->set = TRUE;
    }
  else
    update_text (cb);

  return FALSE;
}

static void
gtk_date_combo_init (GtkDateCombo *combo)
{
  GtkWidget *arrow;
  time_t t;
  struct tm tm;
#ifdef IS_HILDON	  
  GdkPixbuf *pixbuf = gpe_try_find_icon ("qgn_widg_datedit", NULL);
#else
  GdkPixbuf *pixbuf = gpe_try_find_icon ("month_view", NULL);
#endif
  
  time (&t);
  localtime_r (&t, &tm);

  combo->set = TRUE;
  combo->year = tm.tm_year + 1900;
  combo->month = tm.tm_mon;
  combo->day = tm.tm_mday;
 
  combo->ignore_year = FALSE;
  
  if (pixbuf)
    arrow = gtk_image_new_from_pixbuf (pixbuf);
  else
    arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT);
  gtk_widget_show (arrow);

  combo->button = gtk_button_new ();
  combo->entry = gtk_entry_new ();

  update_text (combo);

  gtk_container_add (GTK_CONTAINER (combo->button), arrow);

  gtk_box_pack_start (GTK_BOX (combo), combo->entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (combo), combo->button, FALSE, FALSE, 0);

  GTK_WIDGET_SET_FLAGS (combo->button, GTK_CAN_FOCUS);

  gtk_entry_set_editable (GTK_ENTRY (combo->entry), TRUE);

  gtk_widget_show (combo->button);
  gtk_widget_show (combo->entry);

#if defined(IS_HILDON) && HILDON_VER > 0
  combo->cal = hildon_calendar_new ();
#else
  combo->cal = gtk_calendar_new ();
#endif
  combo->calw = gtk_window_new (GTK_WINDOW_POPUP);
  g_object_ref (combo->calw);
  combo->cal_open = FALSE;
  gtk_widget_set_events (combo->cal, GDK_KEY_PRESS_MASK);
  g_signal_connect (G_OBJECT (combo->calw), "button_press_event",
		    G_CALLBACK (gtk_date_combo_button_press), combo);
  gtk_widget_show (combo->cal);
  gtk_container_add (GTK_CONTAINER (combo->calw), combo->cal);

  gtk_window_set_policy (GTK_WINDOW (combo->calw), FALSE, FALSE, TRUE);
      
  g_signal_connect (G_OBJECT (combo->button), "clicked",
                    G_CALLBACK (drop_calendar), combo);

  g_signal_connect (G_OBJECT (combo->cal),
                    gpe_stylus_mode () ? "day-selected" : "day-selected-double-click",
                    G_CALLBACK (click_calendar), combo);

  g_signal_connect (G_OBJECT (combo->entry), "focus-out-event",
                    G_CALLBACK (verify_date), combo);
}

static GtkHBoxClass *parent_class = NULL;

static void
gtk_date_combo_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
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
gtk_date_combo_show (GtkWidget *widget)
{
  GtkDateCombo *combo;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_DATE_COMBO (widget));

  GTK_WIDGET_CLASS (parent_class)->show (widget);

  combo = GTK_DATE_COMBO (widget);
  gtk_widget_show (combo->entry);
  gtk_widget_show (combo->button);
}

static void
gtk_date_combo_destroy (GtkObject *object)
{
  GtkDateCombo *combo;
  combo = GTK_DATE_COMBO (object);
  if (combo->calw)
    {
      gtk_widget_destroy (combo->calw);
      g_object_unref (combo->calw);
      combo->calw = NULL;
    }
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gtk_date_combo_class_init (GtkDateComboClass * klass)
{
  GtkObjectClass *oclass;
  GtkWidgetClass *widget_class;

  parent_class = g_type_class_peek_parent (klass);
  oclass = (GtkObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  widget_class->size_allocate = gtk_date_combo_size_allocate;
  widget_class->show = gtk_date_combo_show;

  oclass->destroy = gtk_date_combo_destroy;

  my_signals[0] = g_signal_new ("changed",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (struct _GtkDateComboClass, changed),
				NULL, NULL,
				gtk_marshal_VOID__VOID,
				G_TYPE_NONE, 0);
}

GType
gtk_date_combo_get_type (void)
{
  static GType date_combo_type = 0;

  if (! date_combo_type)
    {
      static const GTypeInfo date_combo_info =
        {
          sizeof (GtkDateComboClass),
	  NULL,		/* base_init */
	  NULL,		/* base_finalize */
          (GClassInitFunc) gtk_date_combo_class_init,
	  NULL,		/* class_finalize */
	  NULL,		/* class_data */
	  sizeof (GtkDateCombo),
	  0,		/* n_preallocs */
          (GInstanceInitFunc) gtk_date_combo_init,
        };

      date_combo_type = g_type_register_static (GTK_TYPE_HBOX, "GtkDateCombo",
						&date_combo_info, 0);
    }
  return date_combo_type;
}

GtkWidget *
gtk_date_combo_new (void)
{
  return g_object_new (gtk_date_combo_get_type (), NULL);
}

void
gtk_date_combo_set_date (GtkDateCombo *dp, guint year, guint month, guint day)
{
  dp->year = year;
  dp->month = month;
  dp->day = day;
  update_text (dp);
}

void
gtk_date_combo_clear (GtkDateCombo *dp)
{
  dp->set = FALSE;
  gtk_entry_set_text (GTK_ENTRY (dp->entry), "");
}

void
gtk_date_combo_week_starts_monday (GtkDateCombo *combo, gboolean yes)
{
#if defined(IS_HILDON) && HILDON_VER > 0
  hildon_calendar_set_display_options (HILDON_CALENDAR (combo->cal), 
				HILDON_CALENDAR_SHOW_DAY_NAMES | 
				HILDON_CALENDAR_SHOW_HEADING |
				(yes ? HILDON_CALENDAR_WEEK_START_MONDAY : 0));
#else
  gtk_calendar_display_options (GTK_CALENDAR (combo->cal), 
				GTK_CALENDAR_SHOW_DAY_NAMES | 
				GTK_CALENDAR_SHOW_HEADING |
				(yes ? GTK_CALENDAR_WEEK_START_MONDAY : 0));
#endif
}

void
gtk_date_combo_ignore_year (GtkDateCombo *combo, gboolean yes)
{
  if (combo->ignore_year != yes)
    {
      combo->ignore_year = yes;
      if (combo->year == 0) 
        combo->year = 2000;
      update_text (combo);
    }
}
