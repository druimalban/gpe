/* calendar-delete-dialog.h - Calendar delete dialog implementation.
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

#include <gtk/gtk.h>
#include <gpe/errorbox.h>

#include "globals.h"
#include "calendars-widgets.h"
#include "calendar-delete-dialog.h"

struct _CalendarDeleteDialog
{
  GtkDialog dialog;

  EventCalendar *ec;

  GtkBox *vbox;

  GtkToggleButton *delete_toggle;
  GtkToggleButton *reparent_toggle;
  GtkComboBox *reparent_combo;
};

struct _CalendarDeleteDialogClass
{
  GtkDialogClass gtk_dialog_class;
};

static void calendar_delete_dialog_class_init (gpointer klass,
					       gpointer klass_data);
static void calendar_delete_dialog_dispose (GObject *obj);
static void calendar_delete_dialog_finalize (GObject *object);
static void calendar_delete_dialog_init (GTypeInstance *instance,
				       gpointer klass);

static GtkWidgetClass *calendar_delete_dialog_parent_class;

GType
calendar_delete_dialog_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (CalendarDeleteDialogClass),
	NULL,
	NULL,
	calendar_delete_dialog_class_init,
	NULL,
	NULL,
	sizeof (struct _CalendarDeleteDialog),
	0,
	calendar_delete_dialog_init
      };

      type = g_type_register_static (gtk_dialog_get_type (),
				     "CalendarDeleteDialog", &info, 0);
    }

  return type;
}

static void
calendar_delete_dialog_class_init (gpointer klass, gpointer klass_data)
{
  GObjectClass *object_class;

  calendar_delete_dialog_parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = calendar_delete_dialog_finalize;
  object_class->dispose = calendar_delete_dialog_dispose;
}

static void
calendar_delete_dialog_dispose (GObject *obj)
{
  /* Chain up to the parent class.  */
  G_OBJECT_CLASS (calendar_delete_dialog_parent_class)->dispose (obj);
}

static void
calendar_delete_dialog_finalize (GObject *object)
{
  CalendarDeleteDialog *calendar_delete_dialog
    = CALENDAR_DELETE_DIALOG (object);

  if (calendar_delete_dialog->ec)
    g_object_unref (calendar_delete_dialog->ec);

  G_OBJECT_CLASS (calendar_delete_dialog_parent_class)->finalize (object);
}

static void
delete_clicked (GtkButton *button, gpointer user_data)
{
  CalendarDeleteDialog *d = CALENDAR_DELETE_DIALOG (user_data);

  if (d->reparent_toggle
      && gtk_toggle_button_get_active (d->reparent_toggle))
    {
      EventCalendar *p = 
	calendars_combo_box_get_active (GTK_COMBO_BOX (d->reparent_combo));

      if (event_calendar_valid_parent (d->ec, p))
	{
	  event_calendar_delete (d->ec, FALSE, p);
	  gtk_widget_destroy (GTK_WIDGET (d));
	}
      else
	{
	  char *ec_title = event_calendar_get_title (d->ec);
	  char *p_title = event_calendar_get_title (p);

	  gpe_error_box_fmt
	    (_(" %s cannot be the new parent of %s's "
	       "constituents: this would create a loop (try being your "
	       "own grandparent).\n"
	       "Please select a calendar which is not a "
	       "descendent of %s."),
	     p_title, ec_title, ec_title);

	  g_free (ec_title);
	  g_free (p_title);
	}
    }
  else
    {
      event_calendar_delete (d->ec, TRUE, NULL);
      gtk_widget_destroy (GTK_WIDGET (d));
    }
}

void
calendar_delete_dialog_set_calendar (CalendarDeleteDialog *d,
				     EventCalendar *ec)
{
  if (d->vbox)
    gtk_container_remove (GTK_CONTAINER (GTK_DIALOG (d)->vbox),
			  GTK_WIDGET (d->vbox));
  if (d->ec)
    g_object_unref (d->ec);

  g_object_ref (ec);
  d->ec = ec;

  d->vbox = GTK_BOX (gtk_vbox_new (FALSE, 3));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (d)->vbox), GTK_WIDGET (d->vbox),
		      FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (d->vbox));
  
  char *title = event_calendar_get_title (ec);
  char *s = g_strdup_printf (_("Delete Calendar %s"), title);
  gtk_window_set_title (GTK_WINDOW (d), s);
  g_free (s);

  GSList *cals = event_calendar_list_calendars (ec);
  GSList *events = event_calendar_list_events (ec);
  s = g_strdup_printf ("<i>%s</i> contains %d calendars and %d events",
		       title, g_slist_length (cals),
		       g_slist_length (events));

  GSList *i;
  for (i = cals; i; i = i->next)
    g_object_unref (i->data);
  g_slist_free (cals);
  for (i = events; i; i = i->next)
    g_object_unref (i->data);
  g_slist_free (events);

  GtkWidget *l = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (l), s);
  g_free (s);
  gtk_box_pack_start (d->vbox, l, FALSE, FALSE, 0);
  gtk_widget_show (l);

  if (cals || events)
    {
      char *s = g_strdup_printf (_("Delete %s%s%s"),
				 cals ? "calendars" : "",
				 cals && events ? " and " : "",
				 events ? "events" : "");
      GtkWidget *r = gtk_radio_button_new_with_label (NULL, s);
      g_free (s);
      d->delete_toggle = GTK_TOGGLE_BUTTON (r);
      gtk_box_pack_start (d->vbox, r, FALSE, FALSE, 0);
      gtk_widget_show (r);

      s = g_strdup_printf (_("Reparent %s%s%s"),
			   cals ? "calendars" : "",
			   cals && events ? " and " : "",
			   events ? "events" : "");
      r = gtk_radio_button_new_with_label_from_widget
	(GTK_RADIO_BUTTON (r), s);
      g_free (s);
      d->reparent_toggle = GTK_TOGGLE_BUTTON (r);
      gtk_box_pack_start (d->vbox, r, FALSE, FALSE, 0);
      gtk_widget_show (r);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (r), TRUE);

      GtkWidget *w = calendars_combo_box_new (event_db);
      d->reparent_combo = GTK_COMBO_BOX (w);
      gtk_box_pack_start (d->vbox, w, FALSE, FALSE, 0);
      gtk_widget_show (w);

      EventCalendar *p = event_calendar_get_parent (ec);
      if (! p)
	p = event_db_get_default_calendar (event_db, NULL);
      else
	g_object_ref (p);
      calendars_combo_box_set_active (GTK_COMBO_BOX (w), p);
      g_object_unref (p);
    }
  else
    {
      d->delete_toggle = NULL;
      d->reparent_toggle = NULL;
      d->reparent_combo = NULL;
    }

  g_free (title);
}

static void
calendar_delete_dialog_init (GTypeInstance *instance, gpointer klass)
{
  CalendarDeleteDialog *d = CALENDAR_DELETE_DIALOG (instance);

  gtk_window_set_position (GTK_WINDOW (instance),
			   GTK_WIN_POS_CENTER_ON_PARENT);

  /* Fill the action box.  */
  GtkWidget *b = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  g_signal_connect (G_OBJECT (b), "clicked",
		    G_CALLBACK (delete_clicked), d);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (d)->action_area), GTK_WIDGET (b),
		    FALSE, FALSE, 0);
  gtk_widget_show (b);

  b = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  g_signal_connect_swapped (G_OBJECT (b), "clicked",
			    G_CALLBACK (gtk_widget_destroy), d);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (d)->action_area), GTK_WIDGET (b),
		    FALSE, FALSE, 0);
  gtk_widget_show (b);
}

GtkWidget *
calendar_delete_dialog_new (EventCalendar *ec)
{
  CalendarDeleteDialog *calendar_delete
    = CALENDAR_DELETE_DIALOG (g_object_new (calendar_delete_dialog_get_type (),
					    NULL));

  calendar_delete_dialog_set_calendar (calendar_delete, ec);

  return GTK_WIDGET (calendar_delete);
}
