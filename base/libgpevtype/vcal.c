/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <libintl.h>
#include <errno.h>
#include <string.h>

#include <mimedir/mimedir-vcal.h>
#include <mimedir/mimedir-valarm.h>
#include <mimedir/mimedir-vevent.h>

#include "priv.h"

gboolean
cal_import_from_channel (EventCalendar *ec, GIOChannel *channel,
			 GError **error)
{
  GList *callist = mimedir_vcal_read_channel (channel, error);
  if (! callist)
    return FALSE;

  gboolean result = cal_import_from_vmimedir (ec, callist, error);
  mimedir_vcal_free_list (callist);
  return result;
}

gboolean
cal_import_from_vmimedir (EventCalendar *ec, GList *callist, GError **error)
{
  char *err_str = NULL;

  void add_message (GError *error)
    {
      gchar *tmp;
      tmp = g_strdup_printf ("%s%s%s",
			     err_str ?: "", err_str ? "\n" : "",
			     error->message);
      g_free (err_str);
      err_str = tmp;
      g_error_free (error);
    }

  GList *l;
  for (l = callist; l; l = l->next)
    if (l->data && MIMEDIR_IS_VCAL (l->data)) 
      {
	MIMEDirVCal *vcal = l->data;

	GSList *list = mimedir_vcal_get_event_list (vcal);
	GSList *iter;
	for (iter = list; iter; iter = iter->next)
	  {
	    MIMEDirVEvent *vevent = MIMEDIR_VEVENT (iter->data);

	    /* We collapse error messages into a single error message
	       as it can't be handled programatically anyway.  */
	    GError *import_error = NULL;
	    if (! event_import_from_vevent (ec, vevent, NULL, &import_error))
	      add_message (import_error);
	  }
	g_slist_free (list);

	list = mimedir_vcal_get_todo_list (vcal);
	for (iter = list; iter; iter = iter->next)
	  {
	    MIMEDirVTodo *vtodo = MIMEDIR_VTODO (iter->data);

	    GError *import_error = NULL;
	    if (! todo_import_from_vtodo (vtodo, &import_error))
	      add_message (import_error);
	  }
	g_slist_free (list);
      }

  if (err_str)
    {
      g_set_error (error, ERROR_DOMAIN (), 0, err_str);
      g_free (err_str);
      return FALSE;
    }
  return TRUE;
}

char *
cal_export_as_string (EventCalendar *ec)
{
  MIMEDirVCal *vcal = mimedir_vcal_new ();

  GSList *l = event_calendar_list_events (ec);
  GSList *i;
  for (i = l; i; i = i->next)
    {
      MIMEDirVEvent *vev = event_export_as_vevent (i->data);
      g_object_unref (i->data);
      mimedir_vcal_add_component (vcal, MIMEDIR_VCOMPONENT (vev));
      g_object_unref (vev);
    }
  g_slist_free (l);

  char *s = mimedir_vcal_write_to_string (vcal);
  g_object_unref (vcal);

  return s;
}

/* THING is either an Event or EventCalendar.  */
static gboolean
save_to_file (GObject *thing, const gchar *filename, GError **error)
{
  char *s;
  if (IS_EVENT (thing))
    s = event_export_as_string (EVENT (thing));
  else
    s = cal_export_as_string (EVENT_CALENDAR (thing));

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

gboolean
cal_export_to_file (EventCalendar *ec, const gchar *filename, GError **error)
{
  return save_to_file (G_OBJECT (ec), filename, error);
}

gboolean
event_export_to_file (Event *ev, const gchar *filename, GError **error)
{
  return save_to_file (G_OBJECT (ev), filename, error);
}

gboolean 
list_export_to_file (GSList *things, const gchar *filename, GError **error)
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
        whole[i] = event_export_as_string (EVENT (thing));
      else
        whole[i] = cal_export_as_string (EVENT_CALENDAR (thing));
      i++;
    }
  s = g_strjoinv ("\n", whole);
  g_strfreev (whole);
    
  FILE *f = fopen (filename, "w");
  if (! f)
    {
      g_set_error (error,
                   ERROR_DOMAIN (), 0,
                   _("Failed to open %s: %s"),
                   filename, g_strerror (errno));
      goto error;
    }

  if (fputs(s, f) == EOF)
    {
      g_set_error (error,
                   ERROR_DOMAIN (), 0,
                   _("Failed to write to %s: %s"),
                   filename, g_strerror (errno));
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
