/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
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
  gchar *tag;
  gchar *vc;
};

static struct tag_map map[] =
  {
    { "summary", NULL },
    { "description", NULL },
    { NULL, NULL }
  };

static gboolean
parse_date (const char *s, struct tm *tm, gboolean *date_only)
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
vevent_interpret_tag (MIMEDirVEvent *event, const char *tag, const char *value)
{
  struct tag_map *t = &map[0];
  while (t->tag)
    {
      if (!strcasecmp (t->tag, tag))
	{
	  g_object_set (G_OBJECT (event), t->vc ? t->vc : t->tag, value, NULL);
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

	  date = mimedir_datetime_new_from_date (tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

	  g_object_set (G_OBJECT (event), "dtstart", date, NULL);
	}

      return TRUE;
    }

  return FALSE;
}

MIMEDirVEvent *
vevent_from_tags (GSList *tags)
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
