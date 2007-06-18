/* alarms.c - Alarm Dialogue Implementation.
   Copyright (C) 2006, 2007 Neal H. Walfield <neal@walfield.org>

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
#include <libintl.h>
#include <stdio.h>
#include <string.h>

#include "alarm-dialog.h"
#include "globals.h"

struct _AlarmDialog
{
  GtkWindow window;

  GtkBox *event_container;

  /* If there are no events, then we display a message to that effect.
     We kill this when an event is added.  */
  GtkWidget *no_alarms;

  /* A list of struct data.  */
  GList *events;
};

struct _AlarmDialogClass
{
  GtkWindowClass gtk_window_class;

  AlarmDialogShowEventFunc show_event;

  guint show_event_signal;
};

/* User data.  */
struct data
{
  AlarmDialog *alarm_dialog;
  GtkWidget *container;
  Event *event;
};

static void alarm_dialog_class_init (gpointer klass, gpointer klass_data);
static void alarm_dialog_dispose (GObject *obj);
static void alarm_dialog_finalize (GObject *object);
static void alarm_dialog_init (GTypeInstance *instance, gpointer klass);
static void alarm_dialog_size_request (GtkWidget *widget,
				       GtkRequisition *requisition);
static void alarm_dialog_show (GtkWidget *widget);

static GtkWidgetClass *alarm_dialog_parent_class;

GType
alarm_dialog_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (AlarmDialogClass),
	NULL,
	NULL,
	alarm_dialog_class_init,
	NULL,
	NULL,
	sizeof (struct _AlarmDialog),
	0,
	alarm_dialog_init
      };

      type = g_type_register_static (gtk_window_get_type (),
				     "AlarmDialog", &info, 0);
    }

  return type;
}

static void
alarm_dialog_class_init (gpointer klass, gpointer klass_data)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;
  AlarmDialogClass *alarm_dialog_class;

  alarm_dialog_parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = alarm_dialog_finalize;
  object_class->dispose = alarm_dialog_dispose;

  widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->size_request = alarm_dialog_size_request;
  widget_class->show = alarm_dialog_show;

  alarm_dialog_class = ALARM_DIALOG_CLASS (klass);
  alarm_dialog_class->show_event_signal
    = g_signal_new ("show-event",
		    G_OBJECT_CLASS_TYPE (object_class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (AlarmDialogClass, show_event),
		    NULL,
		    NULL,
		    g_cclosure_marshal_VOID__POINTER,
		    G_TYPE_NONE,
		    1,
		    G_TYPE_POINTER);
}

static void
alarm_dialog_dispose (GObject *obj)
{
  /* Chain up to the parent class.  */
  G_OBJECT_CLASS (alarm_dialog_parent_class)->dispose (obj);
}

static void
alarm_dialog_finalize (GObject *object)
{
  AlarmDialog *alarm_dialog = ALARM_DIALOG (object);

  GList *i;
  for (i = alarm_dialog->events; i; i = g_list_next (i))
    {
      struct data *data = i->data;
      g_object_unref (EVENT (data->event));
      g_free (data);
    }
  g_list_free (alarm_dialog->events);

  G_OBJECT_CLASS (alarm_dialog_parent_class)->finalize (object);
}

static gboolean
dismiss_clicked (GtkWidget *widget, gpointer d)
{
  AlarmDialog *alarm_dialog = ALARM_DIALOG (d);
  gtk_widget_hide (GTK_WIDGET (alarm_dialog));

  return TRUE;
}

static gboolean ack_event_clicked (GtkWidget *button, gpointer d);

static gboolean
ack_all_events_clicked (GtkWidget *button, gpointer d)
{
  AlarmDialog *alarm_dialog = ALARM_DIALOG (d);

  /* Clear all of the events.  */
  GList *i;
  for (i = alarm_dialog->events; i; i = g_list_next (i))
    ack_event_clicked (button, i->data);
  g_list_free (alarm_dialog->events);

  /* Hide the dialog as well.  */
  gtk_widget_hide (GTK_WIDGET (alarm_dialog));

  return TRUE;
}

static void
alarm_dialog_init (GTypeInstance *instance, gpointer klass)
{
  AlarmDialog *alarm_dialog = ALARM_DIALOG (instance);
  GtkWindow *window = GTK_WINDOW (instance);

  g_signal_connect (G_OBJECT (window), "delete-event",
		    G_CALLBACK (gtk_widget_hide_on_delete), NULL);

  gtk_window_set_title (window, _("Alarms"));
  gpe_set_window_icon (window, "bell");
  gtk_window_set_position (window, GTK_WIN_POS_CENTER_ON_PARENT);

  /* The main vbox.  */
  GtkBox *vbox = GTK_BOX (gtk_vbox_new (FALSE, 0));
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));
  gtk_widget_show (GTK_WIDGET (vbox));

  /* A button box at the bottom.  */
  GtkBox *hbox = GTK_BOX (gtk_hbox_new (FALSE, 0));
  gtk_box_pack_end (vbox, GTK_WIDGET (hbox), FALSE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  GtkBox *buttons = GTK_BOX (gtk_hbox_new (TRUE, 4));
  gtk_container_set_border_width (GTK_CONTAINER (buttons), 2);
  gtk_box_pack_end (hbox, GTK_WIDGET (buttons), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (buttons));

  GtkWidget *button = gtk_button_new_with_label (_("Acknowledge All"));
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (ack_all_events_clicked), alarm_dialog);
  gtk_box_pack_start (buttons, button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Dismiss"));
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (dismiss_clicked), alarm_dialog);
  gtk_box_pack_start (buttons, button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  /* And a separator just above it.  */
  GtkWidget *sep = gtk_hseparator_new ();
  gtk_box_pack_end (vbox, sep, FALSE, TRUE, 0);
  gtk_widget_show (sep);

  /* Finally, the main scrolled window.  */
  GtkScrolledWindow *scrolled_window
    = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));
  gtk_scrolled_window_set_shadow_type (scrolled_window, GTK_SHADOW_NONE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (vbox,
		      GTK_WIDGET (scrolled_window),
		      TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (scrolled_window));
  
  GtkWidget *viewport =
    gtk_viewport_new (gtk_scrolled_window_get_hadjustment (scrolled_window),
		      gtk_scrolled_window_get_vadjustment (scrolled_window));
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);
  gtk_container_add (GTK_CONTAINER (scrolled_window), viewport);
  gtk_widget_show (viewport);

  /* With a vbox in which the events go.  */
  GtkBox *event_container = GTK_BOX (gtk_vbox_new (FALSE, 0));
  alarm_dialog->event_container = event_container;
  gtk_container_add (GTK_CONTAINER (viewport), GTK_WIDGET (event_container));
  gtk_widget_show (GTK_WIDGET (event_container));
}

static void
alarm_dialog_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
  GTK_WIDGET_CLASS (alarm_dialog_parent_class)->size_request (widget,
							      requisition);

  PangoLayout *pl = gtk_widget_create_pango_layout (widget, NULL);
  PangoContext *context = pango_layout_get_context (pl);
  PangoFontMetrics *metrics
    = pango_context_get_metrics (context,
				 widget->style->font_desc,
				 pango_context_get_language (context));

  requisition->width
    = CLAMP (MIN (requisition->width, 200),
	     80 * pango_font_metrics_get_approximate_char_width (metrics)
	     / PANGO_SCALE,
	     gdk_screen_height () * 3 / 4);

  requisition->height
    = CLAMP (MIN (requisition->height, 200),
	     12 * (pango_font_metrics_get_ascent (metrics)
		   + pango_font_metrics_get_descent (metrics)) / PANGO_SCALE,
	     gdk_screen_height () * 3 / 4);

  pango_font_metrics_unref (metrics);
  g_object_unref (pl);
}

static void
consider_no_alarm_label (AlarmDialog *alarm_dialog)
{
  if (! alarm_dialog->events && ! alarm_dialog->no_alarms)
    {
      alarm_dialog->no_alarms = gtk_label_new (_("No alarms have gone off."));
      gtk_box_pack_start (alarm_dialog->event_container,
			  alarm_dialog->no_alarms, TRUE, TRUE, 0);
      gtk_widget_show (alarm_dialog->no_alarms);
    }
  else if (alarm_dialog->events && alarm_dialog->no_alarms)
    {
      gtk_container_remove (GTK_CONTAINER (alarm_dialog->event_container),
			    alarm_dialog->no_alarms);
      alarm_dialog->no_alarms = NULL;
    }
}

static void
alarm_dialog_show (GtkWidget *widget)
{
  AlarmDialog *alarm_dialog = ALARM_DIALOG (widget);

  consider_no_alarm_label (alarm_dialog);

  GTK_WIDGET_CLASS (alarm_dialog_parent_class)->show (widget);
  
}

static gboolean
show_event_clicked (GtkWidget *button, gpointer d)
{
  struct data *data = d;
  AlarmDialog *alarm_dialog = ALARM_DIALOG (data->alarm_dialog);
  Event *ev = EVENT (data->event);

  GValue args[2];
  GValue rv;

  args[0].g_type = 0;
  g_value_init (&args[0], G_TYPE_FROM_INSTANCE (G_OBJECT (alarm_dialog)));
  g_value_set_instance (&args[0], alarm_dialog);
        
  args[1].g_type = 0;
  g_value_init (&args[1], G_TYPE_POINTER);
  g_value_set_pointer (&args[1], ev);

  g_signal_emitv (args,
		  ALARM_DIALOG_GET_CLASS (alarm_dialog)->show_event_signal,
		  0, &rv);

  return TRUE;
}

static gboolean
ack_event_clicked (GtkWidget *button, gpointer d)
{
  struct data *data = d;

  consider_no_alarm_label (data->alarm_dialog);

  data->alarm_dialog->events = g_list_remove (data->alarm_dialog->events, d);
  gtk_container_remove (GTK_CONTAINER (data->alarm_dialog->event_container),
			data->container);
  event_acknowledge (data->event, NULL);
  soundgen_alarm_stop();
  g_object_unref (data->event);
  g_free (data);

  return TRUE;
}

static PangoLayout *
event_get_layout (Event *ev, GtkWidget *widget, int width)
{
  PangoLayout *pl = gtk_widget_create_pango_layout (GTK_WIDGET (widget), NULL);

  char *str (char *s)
    {
      if (! s || ! *s)
	return NULL;

      char *p;
      /* Kill any trailing new lines.  */
      for (p = s + strlen (s) - 1; p > s && *p == '\n'; p --)
	*p = '\0';
      return s;
    }
      
  char *summary = str (event_get_summary (ev, NULL));
  char *description = str (event_get_description (ev, NULL));
  char *location = str (event_get_location (ev, NULL));
  char *buffer
    = g_strdup_printf (_("%s%s%s%s%s%s%s%s"),
		       summary ? _("<b>Summary</b>: ") : "",
		       summary ?: "",
		       summary && description ? "\n" : "",
		       description ? _("<b>Description</b>: ") : "",
		       description ?: "",
		       (summary || description) && location ? "\n" : "",
		       location ? _("<b>Location</b>: ") : "",
		       location ?: "");
  g_free (summary);
  g_free (description);
  g_free (location);

  pango_layout_set_width (pl,
			  (width > 0 ? width : widget->allocation.width)
			  * PANGO_SCALE);
  pango_layout_set_markup (pl, buffer, -1);

  g_free (buffer);

  return pl;
}

static gint
event_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer d)
{
  struct data *data = d;
  Event *ev = EVENT (data->event);

  PangoLayout *pl = event_get_layout (ev, widget, 0);

  gtk_paint_layout (widget->style,
		    widget->window,
		    GTK_WIDGET_STATE (widget),
		    FALSE,
		    &event->area,
		    widget,
		    "label",
		    0, 0,
		    pl);

  g_object_unref (pl);

  return TRUE;
}

static void
event_size_request (GtkWidget *widget,
		    GtkRequisition *requisition,
		    gpointer d)
{
  struct data *data = d;
  Event *ev = EVENT (data->event);

  PangoLayout *pl = event_get_layout (ev, widget, requisition->width);

  PangoRectangle pr;
  pango_layout_get_pixel_extents (pl, NULL, &pr);
  requisition->height = pr.height;

  g_object_unref (pl);
}


void
alarm_dialog_add_event (AlarmDialog *alarm_dialog, Event *event)
{
  /*
     +-area------------------------------------------------+
     |+-vbox-------+ +-----------------------+ +---------+ |
     ||Show Event  | | Summary:              | |    Date | |
     ||Acknowledge | | Description:	     | |         | |
     ||Sleep       | | Location:             | |         | |
     |+------------+ +-----------------------+ +---------+ |
     +-----------------------------------------------------+

  */

  struct data *data = g_malloc (sizeof (struct data));
  data->alarm_dialog = alarm_dialog;
  data->event = event;

  GtkBox *area = GTK_BOX (gtk_hbox_new (FALSE, 2));
  data->container = GTK_WIDGET (area);
  gtk_box_pack_start (alarm_dialog->event_container, GTK_WIDGET (area),
		      FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (area));

  GtkBox *buttons = GTK_BOX (gtk_vbutton_box_new ());
  gtk_button_box_set_layout (GTK_BUTTON_BOX (buttons), GTK_BUTTONBOX_START);
  gtk_box_pack_start (area, GTK_WIDGET (buttons), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (buttons));

  GtkWidget *button = gtk_button_new_with_label (_("Show Event"));
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (show_event_clicked), data);
  gtk_box_pack_start (buttons, GTK_WIDGET (button), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (button));

  button = gtk_button_new_with_label (_("Acknowledge"));
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (ack_event_clicked), data);
  gtk_box_pack_start (buttons, GTK_WIDGET (button), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (button));

#if 0
  button = gtk_button_new_with_label (_("Sleep:"));
  gtk_box_pack_start (buttons, GTK_WIDGET (button), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (button));

  GtkBox *sleep_box = GTK_BOX (gtk_hbox_new (FALSE, 2));
  gtk_box_pack_start (buttons, GTK_WIDGET (sleep_box), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (sleep_box));

  GtkSpinButton *spin
    = GTK_SPIN_BUTTON (gtk_spin_button_new_with_range (1, 1000, 1));
  gtk_spin_button_set_value (spin, 15);
  gtk_box_pack_start (sleep_box, GTK_WIDGET (spin), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (spin));

  GtkComboBox *combo = GTK_COMBO_BOX (gtk_combo_box_new_text ());
  gtk_combo_box_append_text (combo, _("Minutes"));
  gtk_combo_box_append_text (combo, _("Hours"));
  gtk_combo_box_append_text (combo, _("Days"));
  /* Make minutes the default.  */
  gtk_combo_box_set_active (combo, 0);
  gtk_box_pack_start (sleep_box, GTK_WIDGET (combo), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (combo));
#endif

  GtkWidget *draw = gtk_drawing_area_new ();
  g_signal_connect (G_OBJECT (draw), "expose_event",
                    G_CALLBACK (event_expose_event), data);
  g_signal_connect (G_OBJECT (draw), "size_request",
                    G_CALLBACK (event_size_request), data);
  gtk_box_pack_start (area, draw, TRUE, TRUE, 0);
  gtk_widget_show (draw);

  time_t s = event_get_start (event);
  struct tm start;
  localtime_r (&s, &start);
  char *fmt;
  gboolean dealloc;
  if (event_get_untimed (event))
    {
      fmt = "%A\n%B %d";
      dealloc = FALSE;
    }
  else
    {
      fmt = g_strdup_printf ("%%A\n%%B %%d\n%s", TIMEFMT);
      dealloc = TRUE;
    }
  char *buffer = strftime_strdup_utf8_utf8 (fmt, &start);
  if (dealloc)
    g_free (fmt);

  GtkLabel *label = GTK_LABEL (gtk_label_new (buffer));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.0);
  gtk_label_set_justify (label, GTK_JUSTIFY_RIGHT);
  g_free (buffer);
  gtk_box_pack_start (area, GTK_WIDGET (label), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (label));

  /* Add to the list of events.  */
  alarm_dialog->events = g_list_prepend (alarm_dialog->events, data);

  consider_no_alarm_label (alarm_dialog);

  /* Take a reference for our self.  */
  g_object_ref (event);
}

GtkWidget *
alarm_dialog_new (void)
{
  AlarmDialog *alarm_dialog
    = ALARM_DIALOG (g_object_new (alarm_dialog_get_type (), NULL));

  return GTK_WIDGET (alarm_dialog);
}
