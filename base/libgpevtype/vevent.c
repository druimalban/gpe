/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <libintl.h>
#include <assert.h>

#include <gpe/vevent.h>

struct tag_map
{
  GType type;
  gchar *tag;
  gchar *vc;
};

static struct tag_map map[] = {
  {G_TYPE_STRING, "summary", NULL},
  {G_TYPE_STRING, "description", NULL},
  {G_TYPE_STRING, "eventid", "uid"},
  {G_TYPE_INT, "recur", "recurrence"},
  {G_TYPE_INT, "rcount", "repeat"},
  {G_TYPE_INT, "sequence", NULL},
  {G_TYPE_INT, "modified", "dtstamp"},
  {G_TYPE_INT, "duration", NULL},
  {G_TYPE_INVALID, NULL, NULL}
};

static gboolean
parse_date (const char *s, struct tm *tm, gboolean * date_only)
{
  char *p;

  memset (tm, 0, sizeof (*tm));

  p = strptime (s, "%Y-%m-%d", tm);
  if (p == NULL)
    return FALSE;

  p = strptime (p, " %H:%M", tm);

  if (date_only)
    *date_only = (p == NULL) ? TRUE : FALSE;

  return TRUE;
}

static gboolean
vevent_interpret_tag (MIMEDirVEvent * event, const char *tag,
		      const char *value)
{
  struct tag_map *t = &map[0];
  while (t->tag)
    {
      if (!strcasecmp (t->tag, tag))
        {
          if (t->type == G_TYPE_STRING)
            g_object_set (G_OBJECT (event), t->vc ? t->vc : t->tag, value,
                  NULL);
          else if (t->type == G_TYPE_INT)
            g_object_set (G_OBJECT (event), t->vc ? t->vc : t->tag,
                  atoi (value), NULL);
          else
            abort ();
          return TRUE;
        }
      t++;
    }

  if (!strcasecmp (tag, "start"))
    {
      struct tm tm;
      gboolean date_only;

      if (parse_date (value, &tm, &date_only))
        {
          MIMEDirDateTime *date;
    
          date =
            mimedir_datetime_new_from_date (tm.tm_year + 1900, tm.tm_mon + 1,
                            tm.tm_mday);
          if (!date_only)
            {
              mimedir_datetime_set_time (date, tm.tm_hour, tm.tm_min,
                         tm.tm_sec);
              date->timezone = MIMEDIR_DATETIME_UTC;
            }
    
          g_object_set (G_OBJECT (event), "dtstart", date, NULL);
        }
      else
	fprintf (stderr, "couldn't parse date '%s'\n", value);

      return TRUE;
    }

  return FALSE;
}

MIMEDirVEvent *
vevent_from_tags (GSList * tags)
{
  MIMEDirVEvent *vevent = mimedir_vevent_new ();

  while (tags)
    {
      gpe_tag_pair *p = tags->data;

      vevent_interpret_tag (vevent, p->tag, p->value);

      tags = tags->next;
    }

  return vevent;
}

GSList *
vevent_to_tags (MIMEDirVEvent * vevent)
{
  GSList *data = NULL;
  struct tag_map *t = &map[0];
  MIMEDirDateTime *date;

  while (t->tag)
    {
      if (t->type == G_TYPE_STRING)
        {
          gchar *value;
    
          g_object_get (G_OBJECT (vevent), t->vc ? t->vc : t->tag, &value,
                NULL);
    
          if (value)
            data = gpe_tag_list_prepend (data, t->tag, g_strstrip (value));
        }
      else if (t->type == G_TYPE_INT)
        {
          gint value;
    
          g_object_get (G_OBJECT (vevent), t->vc ? t->vc : t->tag, &value,
                NULL);
    
          if (value != 0)
            data =
              gpe_tag_list_prepend (data, t->tag,
                        g_strdup_printf ("%d", value));
        }
      else
        abort ();

      t++;
    }

  g_object_get (G_OBJECT (vevent), "dtstart", &date, NULL);
  if (date)
    {
      gchar buf[256];
      MIMEDirDateTime *end_date;
      time_t start_t, end_t;

      mimedir_datetime_to_utc (date);

      data =
    	gpe_tag_list_prepend (data, "start",
                              mimedir_datetime_to_string (date));

      g_object_get (G_OBJECT (vevent), "dtend", &end_date, NULL);
      if (end_date)
        {
          unsigned int duration;
    
          start_t = mimedir_datetime_get_time_t (date);
          end_t = mimedir_datetime_get_time_t (end_date);
    
          duration = end_t - start_t;
          sprintf (buf, "%d", duration);
          data = gpe_tag_list_prepend (data, "duration", g_strdup (buf));
        }
    }

  return data;
}
