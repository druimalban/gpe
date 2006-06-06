/* calendar-edit-dialog.h - Calendar edit dialog implementation.
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

#include <gpe/event-db.h>
#include <gdk/gdk.h>
#include <gpe/colordialog.h>

#include "calendar-edit-dialog.h"
#include "calendars-widgets.h"
#include "globals.h"

struct _CalendarEditDialog
{
  GtkDialog dialog;

  EventCalendar *ec;

  GtkEntry *title;
  GtkEntry *description;

  GtkWidget *parent_check;
  GtkComboBox *parent;

  GdkColor color;
  GtkButton *color_button;

  GtkComboBox *mode;

  GtkBox *url_box;
  GtkEntry *url;
  GtkEntry *username;
  GtkEntry *password;

  GtkBox *sync_box;
  GtkSpinButton *sync_interval;
  GtkComboBox *sync_interval_units;
};

struct _CalendarEditDialogClass
{
  GtkDialogClass gtk_dialog_class;
};

static void calendar_edit_dialog_class_init (gpointer klass,
					     gpointer klass_data);
static void calendar_edit_dialog_dispose (GObject *obj);
static void calendar_edit_dialog_finalize (GObject *object);
static void calendar_edit_dialog_init (GTypeInstance *instance,
				       gpointer klass);

static GtkWidgetClass *calendar_edit_dialog_parent_class;

GType
calendar_edit_dialog_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (CalendarEditDialogClass),
	NULL,
	NULL,
	calendar_edit_dialog_class_init,
	NULL,
	NULL,
	sizeof (struct _CalendarEditDialog),
	0,
	calendar_edit_dialog_init
      };

      type = g_type_register_static (gtk_dialog_get_type (),
				     "CalendarEditDialog", &info, 0);
    }

  return type;
}

static void
calendar_edit_dialog_class_init (gpointer klass, gpointer klass_data)
{
  GObjectClass *object_class;

  calendar_edit_dialog_parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = calendar_edit_dialog_finalize;
  object_class->dispose = calendar_edit_dialog_dispose;
}

static void
calendar_edit_dialog_dispose (GObject *obj)
{
  /* Chain up to the parent class.  */
  G_OBJECT_CLASS (calendar_edit_dialog_parent_class)->dispose (obj);
}

static void
calendar_edit_dialog_finalize (GObject *object)
{
  CalendarEditDialog *calendar_edit_dialog = CALENDAR_EDIT_DIALOG (object);

  if (calendar_edit_dialog->ec)
    g_object_unref (calendar_edit_dialog->ec);

  G_OBJECT_CLASS (calendar_edit_dialog_parent_class)->finalize (object);
}

static void
parent_combo_changed (GtkComboBox *widget, gpointer user_data)
{
  CalendarEditDialog *d = CALENDAR_EDIT_DIALOG (user_data);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->parent_check), TRUE);
}

static void
parent_check_toggled (GtkToggleButton *togglebutton,
		      gpointer user_data) 
{
  if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton)))
    gtk_combo_box_set_active (GTK_COMBO_BOX (user_data), -1);
}

static void
set_color (CalendarEditDialog *d, const GdkColor *c)
{
  d->color = *c;
  gtk_widget_modify_bg (GTK_WIDGET (d->color_button),
			GTK_STATE_NORMAL, c);
  gtk_widget_modify_bg (GTK_WIDGET (d->color_button),
			GTK_STATE_ACTIVE, c);
  gtk_widget_modify_bg (GTK_WIDGET (d->color_button),
			GTK_STATE_PRELIGHT, c);
  gtk_widget_modify_bg (GTK_WIDGET (d->color_button),
			GTK_STATE_SELECTED, c);
  gtk_widget_modify_bg (GTK_WIDGET (d->color_button),
			GTK_STATE_INSENSITIVE, c);
}

static void
color_button_clicked (GtkWidget *widget, gpointer data)
{
  CalendarEditDialog *d = CALENDAR_EDIT_DIALOG (data);

  GtkWidget *w
    = gpe_color_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (widget)),
			    0, "white");
  gpe_color_dialog_set_color_gdk (GPE_COLOR_DIALOG (w), &d->color);

  if (gtk_dialog_run (GTK_DIALOG (w)) == GTK_RESPONSE_ACCEPT)
    {
      const GdkColor *c
	= gpe_color_dialog_get_color_gdk (GPE_COLOR_DIALOG (w));
      if (c)
	set_color (d, c);
    }

  gtk_widget_destroy (GTK_WIDGET (w));
}

static void
mode_changed (GtkWidget *widget, gpointer data)
{
  switch (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)))
    {
    case 0:
      gtk_widget_hide (GTK_WIDGET (CALENDAR_EDIT_DIALOG (data)->url_box));
      gtk_widget_hide (GTK_WIDGET (CALENDAR_EDIT_DIALOG (data)->sync_box));
      break;
    case 2:
      gtk_widget_show (GTK_WIDGET (CALENDAR_EDIT_DIALOG (data)->url_box));
      gtk_widget_hide (GTK_WIDGET (CALENDAR_EDIT_DIALOG (data)->sync_box));
      break;
    default:
      gtk_widget_show (GTK_WIDGET (CALENDAR_EDIT_DIALOG (data)->url_box));
      gtk_widget_show (GTK_WIDGET (CALENDAR_EDIT_DIALOG (data)->sync_box));
      break;
    }
}

/* Units of synchronization.  */
static int units[] = { 365 * 24 * 60 * 60,
		       30 * 24 * 60 * 60,
		       7 * 24 * 60 * 60,
		       24 * 60 * 60,
		       60 * 60 };

static void
response (GtkDialog *dialog, gint response_id, gpointer user_data)
{
  if (response_id != GTK_RESPONSE_ACCEPT)
    return;

  CalendarEditDialog *d = CALENDAR_EDIT_DIALOG (dialog);

  EventCalendar *parent = NULL;
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (d->parent_check)))
    parent = calendars_combo_box_get_active (d->parent);

  int interval = units[gtk_combo_box_get_active (d->sync_interval_units)]
    / gtk_spin_button_get_value_as_int (d->sync_interval);

  if (! d->ec)
    {
      d->ec = event_calendar_new_full (event_db, parent, TRUE,
				       gtk_entry_get_text (d->title),
				       gtk_entry_get_text (d->description),
				       gtk_entry_get_text (d->url),
				       &d->color,
				       gtk_combo_box_get_active (d->mode),
				       interval);
    }
  else
    {
      event_calendar_set_parent (d->ec, parent);
      event_calendar_set_title (d->ec, gtk_entry_get_text (d->title));
      event_calendar_set_description (d->ec,
				      gtk_entry_get_text (d->description));
      event_calendar_set_url (d->ec, gtk_entry_get_text (d->url));
      event_calendar_set_color (d->ec, &d->color);
      event_calendar_set_mode (d->ec, gtk_combo_box_get_active (d->mode));
      event_calendar_set_sync_interval (d->ec, interval);
    }

  event_calendar_set_username (d->ec, gtk_entry_get_text (d->username));
  event_calendar_set_password (d->ec, gtk_entry_get_text (d->password));
}

static void
calendar_edit_dialog_init (GTypeInstance *instance, gpointer klass)
{
  CalendarEditDialog *d = CALENDAR_EDIT_DIALOG (instance);

  gtk_window_set_title (GTK_WINDOW (instance), _("Calendar: New Calendar"));
  gtk_window_set_position (GTK_WINDOW (instance),
			   GTK_WIN_POS_CENTER_ON_PARENT);

  /* A frame.  */
  GtkWidget *frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (d)->vbox), GTK_WIDGET (frame),
		      TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /* Body box.  */
  GtkBox *body = GTK_BOX (gtk_vbox_new (FALSE, 2));
  gtk_container_set_border_width (GTK_CONTAINER (body), 2);
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (body));
  gtk_widget_show (GTK_WIDGET (body));

  /* Title.  */
  GtkBox *hbox = GTK_BOX (gtk_hbox_new (FALSE, 3));
  gtk_box_pack_start (body, GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  GtkWidget *label = gtk_label_new (_("Title:"));
  gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  d->title = GTK_ENTRY (gtk_entry_new ());
  gtk_box_pack_start (hbox, GTK_WIDGET (d->title), TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (d->title));

  /* Description.  */
  hbox = GTK_BOX (gtk_hbox_new (FALSE, 3));
  gtk_box_pack_start (body, GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  label = gtk_label_new (_("Description:"));
  gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  d->description = GTK_ENTRY (gtk_entry_new ());
  gtk_box_pack_start (hbox, GTK_WIDGET (d->description), TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (d->description));

  /* Parent.  */
  hbox = GTK_BOX (gtk_hbox_new (FALSE, 3));
  gtk_box_pack_start (body, GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  d->parent_check = gtk_check_button_new_with_label (_("Parent:"));
  gtk_box_pack_start (hbox, d->parent_check, FALSE, FALSE, 0);
  gtk_widget_show (d->parent_check);

  d->parent = GTK_COMBO_BOX (calendars_combo_box_new ());
  gtk_combo_box_set_active (GTK_COMBO_BOX (d->parent), -1);
  g_signal_connect (G_OBJECT (d->parent), "changed",
		    G_CALLBACK (parent_combo_changed), d);
  gtk_widget_show (GTK_WIDGET (d->parent));
  gtk_box_pack_start (hbox, GTK_WIDGET (d->parent), FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (d->parent_check), "toggled",
		    G_CALLBACK (parent_check_toggled), d->parent);

  /* Color selection.  */
  hbox = GTK_BOX (gtk_hbox_new (FALSE, 3));
  gtk_box_pack_start (body, GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  label = gtk_label_new (_("Color:"));
  gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  
  d->color_button = GTK_BUTTON (gtk_button_new_with_label ("     "));
  gtk_box_pack_start (hbox, GTK_WIDGET (d->color_button), FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (d->color_button), "clicked",
                    G_CALLBACK (color_button_clicked), d);
  gtk_widget_show (GTK_WIDGET (d->color_button));

  /* Initially, "light blue".  */
  d->color.red = 173 * 256;
  d->color.green = 216 * 256;
  d->color.blue = 230 * 256;
  set_color (d, &d->color);

  /* mode.  */
  hbox = GTK_BOX (gtk_hbox_new (FALSE, 3));
  gtk_box_pack_start (body, GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  label = gtk_label_new (_("Type:"));
  gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  d->mode = GTK_COMBO_BOX (gtk_combo_box_new_text ());
  gtk_combo_box_append_text (d->mode, _("Local"));
  gtk_combo_box_append_text (d->mode, _("Subscribe"));
  gtk_combo_box_append_text (d->mode, _("Publish"));
#if 0
  /* Uncomment as we support other modes.  */
  gtk_combo_box_append_text (d->mode, _("Synchronize"));
#endif
  gtk_combo_box_set_active (d->mode, 0);
  g_signal_connect (d->mode, "changed",
		    G_CALLBACK (mode_changed), d);
  gtk_widget_show (GTK_WIDGET (d->mode));
  gtk_box_pack_start (hbox, GTK_WIDGET (d->mode), FALSE, FALSE, 0);

  /* URL.  */
  d->url_box = GTK_BOX (gtk_vbox_new (FALSE, 3));
  gtk_box_pack_start (body, GTK_WIDGET (d->url_box), FALSE, FALSE, 0);
  /* Don't show initially.  */

  hbox = GTK_BOX (gtk_hbox_new (FALSE, 3));
  gtk_box_pack_start (d->url_box, GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  label = gtk_label_new (_("URL:"));
  gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  d->url = GTK_ENTRY (gtk_entry_new ());
  gtk_box_pack_start (hbox, GTK_WIDGET (d->url), TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (d->url));

  hbox = GTK_BOX (gtk_hbox_new (FALSE, 3));
  gtk_box_pack_start (d->url_box, GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  label = gtk_label_new (_("Username:"));
  gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  d->username = GTK_ENTRY (gtk_entry_new ());
  gtk_box_pack_start (hbox, GTK_WIDGET (d->username), TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (d->username));

  hbox = GTK_BOX (gtk_hbox_new (FALSE, 3));
  gtk_box_pack_start (d->url_box, GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  label = gtk_label_new (_("Password:"));
  gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  d->password = GTK_ENTRY (gtk_entry_new ());
  gtk_entry_set_visibility (d->password, FALSE);
  gtk_box_pack_start (hbox, GTK_WIDGET (d->password), TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (d->password));

  /* Synchronization frequency.  */
  d->sync_box = GTK_BOX (gtk_hbox_new (FALSE, 3));
  gtk_box_pack_start (body, GTK_WIDGET (d->sync_box), FALSE, FALSE, 0);
  /* Don't show initially.  */

  label = gtk_label_new (_("Update:"));
  gtk_box_pack_start (d->sync_box, label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  d->sync_interval
    = GTK_SPIN_BUTTON (gtk_spin_button_new_with_range (1, 99, 1));
  gtk_spin_button_set_numeric (d->sync_interval, TRUE);
  gtk_spin_button_set_value (d->sync_interval, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (d->sync_interval), 3);
  gtk_widget_show (GTK_WIDGET (d->sync_interval));
  gtk_box_pack_start (d->sync_box, GTK_WIDGET (d->sync_interval),
		      FALSE, FALSE, 0);

  d->sync_interval_units = GTK_COMBO_BOX (gtk_combo_box_new_text ());
  gtk_combo_box_append_text (d->sync_interval_units, _("times per year"));
  gtk_combo_box_append_text (d->sync_interval_units, _("times per month"));
  gtk_combo_box_append_text (d->sync_interval_units, _("times per week"));
  gtk_combo_box_append_text (d->sync_interval_units, _("times per day"));
  gtk_combo_box_append_text (d->sync_interval_units, _("times per hour"));
  gtk_combo_box_set_active (d->sync_interval_units, 1);
  gtk_widget_show (GTK_WIDGET (d->sync_interval_units));
  gtk_box_pack_start (d->sync_box, GTK_WIDGET (d->sync_interval_units),
		      FALSE, FALSE, 0);

  /* And fill the action box.  */
  gtk_dialog_add_button (GTK_DIALOG (d), GTK_STOCK_SAVE,
			 GTK_RESPONSE_ACCEPT);
  /* In theory, we'd like to connect to the save button's click
     handler.  We'd also like to allow gtk_destroy_widget to be
     attached to this dialog.  This is a fundamental conflict as the
     GtkDialog implementation makes the reponse signal run before any
     normal signals.  So, we do this instead.  */
  g_signal_connect_closure_by_id
    ((gpointer) d, g_signal_lookup ("response", GTK_TYPE_DIALOG), 0,
     g_cclosure_new_object (G_CALLBACK (response), G_OBJECT (d)), FALSE);

  gtk_dialog_add_button (GTK_DIALOG (d), GTK_STOCK_CANCEL,
			 GTK_RESPONSE_REJECT);
}

EventCalendar *
calendar_edit_dialog_get_calendar (CalendarEditDialog *d)
{
  return d->ec;
}

GtkWidget *
calendar_edit_dialog_new (EventCalendar *ec)
{
  CalendarEditDialog *calendar_edit
    = CALENDAR_EDIT_DIALOG (g_object_new (calendar_edit_dialog_get_type (),
					  NULL));

  if (ec)
    {
      calendar_edit->ec = ec;
      g_object_ref (ec);

      EventCalendar *p = event_calendar_get_parent (ec);
      if (p)
	{
	  calendars_combo_box_set_active (calendar_edit->parent, p);
	  g_object_unref (p);
	  gtk_toggle_button_set_active
	    (GTK_TOGGLE_BUTTON (calendar_edit->parent_check), TRUE);
	}

      char *s = event_calendar_get_title (ec);
      gtk_entry_set_text (calendar_edit->title, s);
      g_free (s);

      s = event_calendar_get_description (ec);
      gtk_entry_set_text (calendar_edit->description, s);
      g_free (s);

      if (event_calendar_get_color (ec, &calendar_edit->color))
	set_color (calendar_edit, &calendar_edit->color);

      gtk_combo_box_set_active (calendar_edit->mode,
				event_calendar_get_mode (ec));

      s = event_calendar_get_url (ec);
      gtk_entry_set_text (calendar_edit->url, s);
      g_free (s);

      s = event_calendar_get_username (ec);
      gtk_entry_set_text (calendar_edit->username, s);
      g_free (s);

      s = event_calendar_get_password (ec);
      gtk_entry_set_text (calendar_edit->password, s);
      g_free (s);

      int interval = event_calendar_get_sync_interval (ec);
      if (interval)
	{
	  int i;
	  int per;
	  for (i = sizeof (units) / sizeof (units[0]) - 1; i >= 0; i --)
	    if (interval == units[i] || interval <= units[i])
	      {
		per = units[i] / interval;
		break;
	      }

	  if (i == -1)
	    {
	      i = 0;
	      per = 1;
	    }

	  gtk_spin_button_set_value (calendar_edit->sync_interval, per);
	  gtk_combo_box_set_active (calendar_edit->sync_interval_units, i);
	}

      gtk_window_set_title (GTK_WINDOW (calendar_edit), 
			    _("Calendar: Edit Calendar"));
    }

  return GTK_WIDGET (calendar_edit);
}
