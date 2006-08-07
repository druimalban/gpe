/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *               2005 Florian Boor <florian@kernelconcepts.de> 
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtk/gtk.h>
#include <libintl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <fcntl.h>
#include <errno.h>

#include "export-vcal.h"
#include "globals.h"

#include <gpe/errorbox.h>
#include <mimedir/mimedir.h>

#ifdef USE_DBUS
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

static DBusConnection *connection;

#define BLUETOOTH_SERVICE_NAME   "org.handhelds.gpe.bluez"
#define IRDA_SERVICE_NAME   "org.handhelds.gpe.irda"
#endif /* USE_DBUS */

#ifdef IS_HILDON /* Hildon includes */
#include <hildon-fm/hildon-widgets/hildon-file-chooser-dialog.h>
#endif

static MIMEDirVEvent *
export_event_as_vevent (Event *ev)
{
  MIMEDirVEvent *event = mimedir_vevent_new ();

  char *uid = event_get_eventid (ev);
  g_object_set (event, "uid", uid, NULL);
  g_free (uid);

  time_t s = event_get_recurrence_start (ev);
  time_t e = s + event_get_duration (ev);
  struct tm tm;

  MIMEDirDateTime *dtstart;
  MIMEDirDateTime *dtend;
  if (event_get_untimed (ev))
    {
      localtime_r (&s, &tm);
      dtstart = mimedir_datetime_new_from_date (tm.tm_year + 1900,
						tm.tm_mon + 1,
						tm.tm_mday);
      localtime_r (&e, &tm);
      dtend = mimedir_datetime_new_from_date (tm.tm_year + 1900,
					      tm.tm_mon + 1,
					      tm.tm_mday);
      mimedir_vcomponent_set_allday (MIMEDIR_VCOMPONENT (event), TRUE);
    }
  else
    {
      gmtime_r (&s, &tm);
      dtstart = mimedir_datetime_new_from_datetime (tm.tm_year + 1900,
						    tm.tm_mon + 1,
						    tm.tm_mday,
						    tm.tm_hour,
						    tm.tm_min,
						    tm.tm_sec);
      dtstart->timezone = MIMEDIR_DATETIME_UTC;

      gmtime_r (&e, &tm);
      dtend = mimedir_datetime_new_from_datetime (tm.tm_year + 1900,
						  tm.tm_mon + 1,
						  tm.tm_mday,
						  tm.tm_hour,
						  tm.tm_min,
						  tm.tm_sec);
      dtend->timezone = MIMEDIR_DATETIME_UTC;
    }

  g_object_set (event, "dtstart", dtstart, NULL);
  g_object_unref (dtstart);
  g_object_set (event, "dtend", dtend, NULL);
  g_object_unref (dtend);

  int alarm = event_get_alarm (ev);
  if (alarm)
    g_object_set (event, "trigger", &alarm, NULL);

#if 0
  GList *categories = NULL;
  g_object_get (event, "category-list", &categories, NULL);

  GList *i;
  for (i = categories; i; i = i->next)
    {
      event_add_category (ev, map category name to integer...);
      g_free (i->data);
    }
  g_list_free (categories);
#endif

  char *summary = event_get_summary (ev);
  if (summary)
    {
      if (*summary)
	g_object_set (event, "summary", summary, NULL);
      g_free (summary);
    }

  char *description = event_get_description (ev);
  if (description)
    {
      if (*description)
	g_object_set (event, "description", description, NULL);
      g_free (description);
    }

  char *location = event_get_location (ev);
  if (location)
    {
      if (*location)
	mimedir_vcomponent_set_location (MIMEDIR_VCOMPONENT (event),
					 location, "");
      g_free (location);
    }
    
  g_object_set (event, "sequence", event_get_sequence (ev), NULL);

  if (event_get_recurrence_type (ev) != RECUR_NONE)
    {
      MIMEDirRecurrence *recurrence = mimedir_recurrence_new ();

      MIMEDirRecurrenceFrequency freq;
      switch (event_get_recurrence_type (ev))
	{
	case RECUR_DAILY:
	  freq = RECURRENCE_DAILY;
	  break;
	case RECUR_WEEKLY:
	  freq = RECURRENCE_WEEKLY;
	  break;
	case RECUR_MONTHLY:
	  freq = RECURRENCE_MONTHLY;
	  break;
	case RECUR_YEARLY:
	  freq = RECURRENCE_YEARLY;
	  break;
	default:
	  g_assert_not_reached ();
	}

      g_object_set (G_OBJECT (recurrence), "frequency", freq, NULL);

      /* XXX: Add EXDATE (exceptions) when libmimedir supports it.  */
      /* XXX: Add RDATE (BYDAY, etc.) when libmimedir supports it.  */

      time_t e = event_get_recurrence_end (ev);
      if (e)
	{
	  MIMEDirDateTime *until;
	  if (event_get_untimed (ev))
	    {
	      struct tm tm;
	      localtime_r (&e, &tm);
	      until = mimedir_datetime_new_from_date (tm.tm_year + 1900,
						      tm.tm_mon + 1,
						      tm.tm_mday);
	    }
	  else
	    {
	      struct tm tm;
	      gmtime_r (&e, &tm);
	      until = mimedir_datetime_new_from_datetime (tm.tm_year + 1900,
							  tm.tm_mon + 1,
							  tm.tm_mday,
							  tm.tm_hour,
							  tm.tm_min,
							  tm.tm_sec);
	      until->timezone = MIMEDIR_DATETIME_UTC;
	    }

	  g_object_set (recurrence, "until", until, NULL);
	  g_object_unref (until);
	}

      int count = event_get_recurrence_count (ev);
      if (count)
	g_object_set (recurrence, "count", count, NULL);

      int interval = event_get_recurrence_increment (ev);
      if (interval > 1)
	g_object_set (recurrence, "interval", interval, NULL);

      GSList *byday = event_get_recurrence_byday (ev);
      if (byday)
	{
	  char *s[g_slist_length (byday) + 1];
	  int i;
	  GSList *l;
	  for (l = byday, i = 0; l; l = l->next, i ++)
	    s[i] = byday->data;
	  s[i] = NULL;

	  char *units = g_strjoinv (",", s);

	  g_object_set (recurrence, "unit", RECURRENCE_UNIT_DAY, NULL);
	  g_object_set (recurrence, "units", s, NULL);
	  event_recurrence_byday_free (byday);
	  g_free (units);
	}

      g_object_set (event, "recurrence", recurrence, NULL);
      g_object_unref (recurrence);
    }

  return event;
}

char *
export_event_as_string (Event *ev)
{
  MIMEDirVCal *vcal = mimedir_vcal_new ();

  MIMEDirVEvent *vev = export_event_as_vevent (ev);
  mimedir_vcal_add_component (vcal, MIMEDIR_VCOMPONENT (vev));
  g_object_unref (vev);

  char *s = mimedir_vcal_write_to_string (vcal);
  g_object_unref (vcal);

  return s;
}

char *
export_calendar_as_string (EventCalendar *ec)
{
  MIMEDirVCal *vcal = mimedir_vcal_new ();

  GSList *l = event_calendar_list_events (ec);
  GSList *i;
  for (i = l; i; i = i->next)
    {
      MIMEDirVEvent *vev = export_event_as_vevent (i->data);
      g_object_unref (i->data);
      mimedir_vcal_add_component (vcal, MIMEDIR_VCOMPONENT (vev));
      g_object_unref (vev);
    }
  g_slist_free (l);

  char *s = mimedir_vcal_write_to_string (vcal);
  g_object_unref (vcal);

  return s;
}

void
vcal_do_send_bluetooth (Event *event)
{
#ifdef USE_DBUS
  gchar *vcal;
  DBusMessage *message;
  gchar *filename, *mimetype;
#ifndef HAVE_DBUS_MESSAGE_ITER_GET_BASIC
  DBusMessageIter iter;
#endif
  vcal = export_event_as_string (event);

  message = dbus_message_new_method_call (BLUETOOTH_SERVICE_NAME,
					  "/org/handhelds/gpe/bluez/OBEX",
					  BLUETOOTH_SERVICE_NAME ".OBEX",
					  "ObjectPush");

  filename = "GPE.vcf";
  mimetype = "application/x-vcal";

#ifdef HAVE_DBUS_MESSAGE_ITER_GET_BASIC
  dbus_message_append_args (message, DBUS_TYPE_STRING, &filename,
			    DBUS_TYPE_STRING, &mimetype,
			    DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, 
			    &vcal, strlen (vcal), DBUS_TYPE_INVALID);
#else
  dbus_message_append_iter_init (message, &iter);

  dbus_message_iter_append_string (&iter, filename);
  dbus_message_iter_append_string (&iter, mimetype);
  dbus_message_iter_append_byte_array (&iter, vcal, strlen (vcal));
#endif

  dbus_connection_send (connection, message, NULL);

  g_free (vcal);
#endif /* USE_DBUS */
}

void
vcal_do_send_irda (Event *event)
{
#ifdef USE_DBUS
  gchar *vcal;
  DBusMessage *message;
  DBusMessageIter iter;

  vcal = export_event_as_string (event);

  message = dbus_message_new_method_call (IRDA_SERVICE_NAME,
					  "/org/handhelds/gpe/irda/OBEX",
					  IRDA_SERVICE_NAME ".OBEX",
					  "ObjectPush");

#ifdef HAVE_DBUS_MESSAGE_ITER_GET_BASIC
  dbus_message_iter_init_append (message, &iter);

  dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, "GPE.vcf");
  dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, "application/x-vcal");
  dbus_message_iter_append_fixed_array (&iter, DBUS_TYPE_BYTE, vcal, strlen (vcal));
#else
  dbus_message_append_iter_init (message, &iter);

  dbus_message_iter_append_string (&iter, "GPE.vcf");
  dbus_message_iter_append_string (&iter, "application/x-vcal");
  dbus_message_iter_append_byte_array (&iter, vcal, strlen (vcal));
#endif

  dbus_connection_send (connection, message, NULL);

  g_free (vcal);
#endif /* USE_DBUS */
}

/* THING is either an Event or EventCalendar.  */
static gboolean
save_to_file (GObject *thing, const gchar *filename, GError **error)
{
  char *s;
  if (IS_EVENT (thing))
    s = export_event_as_string (EVENT (thing));
  else
    s = export_calendar_as_string (EVENT_CALENDAR (thing));

  FILE *f = fopen (filename, "w");
  if (! f)
    {
      *error = g_error_new (G_FILE_ERROR, g_file_error_from_errno (errno),
			    _("Opening %s"), filename);
      goto error;
    }

  if (fputs(s, f) == EOF)
    {
      *error = g_error_new (G_FILE_ERROR, g_file_error_from_errno (errno),
			    _("Writing to %s"), filename);
      goto error;
    }
  
  fclose (f);
  g_free (s);
  return TRUE;
  
 error:
  if (f)
    fclose (f);
  g_free (s);
  return FALSE;
}

/* THIS is either an Event or a Calendar.  */
static void
save_as_dialog (GObject *thing)
{
  GtkWidget *filesel;
  const gchar *filename;

  g_object_ref (thing);

  char *summary;
  if (IS_EVENT (thing))
    summary = event_get_summary (EVENT (thing));
  else
    summary = event_calendar_get_title (EVENT_CALENDAR (thing));

  char *suggestion = g_strdup_printf ("%s.ics", summary);
  char *s = suggestion;
  while ((s = strchr (s, ' ')))
    *s = '_';

#ifdef IS_HILDON	
  filesel = hildon_file_chooser_dialog_new(NULL, GTK_FILE_CHOOSER_ACTION_SAVE);
  gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(filesel), suggestion);
#else
  s = g_strdup_printf (_("Save %s as..."), summary);
  filesel = gtk_file_selection_new (s);
  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel), suggestion);
#endif
  
  gtk_widget_show (filesel);
  if (gtk_dialog_run (GTK_DIALOG (filesel)) == GTK_RESPONSE_OK)
    {
#ifdef IS_HILDON
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filesel));
#else
      filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel));
#endif
      GError *error = NULL;
      if (! save_to_file (thing, filename, &error))
	{
	  gpe_error_box_fmt (_("Saving %s: %s"), summary, error->message);
	  g_error_free (error);
	  g_free (summary);
	}
    }
  
  g_free (summary);
  gtk_widget_destroy (filesel);
  g_object_unref (thing);
  return;
}

void
export_event_save_as_dialog (Event *ev)
{
  save_as_dialog (G_OBJECT (ev));
}

void
export_calendar_save_as_dialog (EventCalendar *ec)
{
  save_as_dialog (G_OBJECT (ec));
}

gboolean
export_bluetooth_available (void)
{
#ifdef USE_DBUS
  dbus_bool_t r;

  if (connection == NULL)
    return FALSE;

#ifdef HAVE_DBUS_MESSAGE_ITER_GET_BASIC
  r = dbus_bus_name_has_owner (connection, BLUETOOTH_SERVICE_NAME, NULL);
#else
  r = dbus_bus_service_exists (connection, BLUETOOTH_SERVICE_NAME, NULL);
#endif

  return r ? TRUE : FALSE;
#else
  return FALSE;
#endif /* USE_DBUS */
}

gboolean
export_irda_available (void)
{
#ifdef USE_DBUS
  dbus_bool_t r;

  if (connection == NULL)
    return FALSE;

#ifdef HAVE_DBUS_MESSAGE_ITER_GET_BASIC
  r = dbus_bus_name_has_owner (connection, IRDA_SERVICE_NAME, NULL);
#else
  r = dbus_bus_service_exists (connection, IRDA_SERVICE_NAME, NULL);
#endif

  return r ? TRUE : FALSE;
#else
  return FALSE;
#endif /* USE_DBUS */
}

void
vcal_export_init (void)
{
#ifdef USE_DBUS
  DBusError error;

  dbus_error_init (&error);

  connection = dbus_bus_get (DBUS_BUS_SESSION, &error);
  if (connection)
    dbus_connection_setup_with_g_main (connection, NULL);
#endif /* USE_DBUS */
}

gboolean
export_calendar_to_file (EventCalendar *ec, const gchar *filename)
{
    GError *err = NULL;
    
    g_return_val_if_fail (filename && G_IS_OBJECT (ec), FALSE);
    save_to_file (G_OBJECT (ec), filename, &err);
    if (err)
      {
        g_error_free (err);
        return FALSE;
      }
    return TRUE;
}

gboolean 
export_list_to_file (GSList *things, const gchar *filename)
{
  GSList *iter;
  gchar *s = NULL;
  gint n, i = 0;

  g_return_val_if_fail (things, TRUE); /* nothing to do */
  
  n = g_slist_length (things);
  gchar **whole = g_malloc0 ((n + 1) * sizeof (gchar *));
  
  for (iter = things; iter; iter = iter->next)
    {
      GObject *thing = iter->data;
      if (IS_EVENT (thing))
        whole[i] = export_event_as_string (EVENT (thing));
      else
        whole[i] = export_calendar_as_string (EVENT_CALENDAR (thing));
      i++;
    }
  s = g_strjoinv ("\n", whole);
  g_strfreev (whole);
    
  FILE *f = fopen (filename, "w");
  if (! f)
    {
      g_printerr ("Opening %s: %s", filename, strerror (errno));
      goto error;
    }

  if (fputs(s, f) == EOF)
    {
      g_printerr ("Writing to %s: %s", filename, strerror (errno));
      goto error;
    }
  
  fclose (f);
  g_free (s);
  return TRUE;
  
 error:
  if (f)
    fclose (f);
  g_free (s);
  return FALSE;
}
