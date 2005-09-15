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

#include <mimedir/mimedir-vtodo.h>
#include <gpe/vtodo.h>

#define _(x) gettext (x)

/* this is from todo-db.h for status mapping */
typedef enum
{
  NOT_STARTED,
  IN_PROGRESS,
  COMPLETED,
  ABANDONED
} item_state;

struct tag_map
{
  GType type;
  gchar *tag;
  gchar *vc;
};

static struct tag_map map[] =
  {
    { G_TYPE_STRING, "summary", NULL },
    { G_TYPE_STRING, "description", NULL },
    { G_TYPE_STRING, "todoid", "uid" },
    { G_TYPE_INT, "priority", NULL },
    { G_TYPE_NONE, NULL, NULL }
  };

static gboolean
vtodo_interpret_tag (MIMEDirVTodo *todo, const char *tag, const char *value)
{
  struct tag_map *t = &map[0];
  while (t->tag)
    {
      if (!strcasecmp (t->tag, tag))
	{
	  if (t->type == G_TYPE_STRING)
	    g_object_set (G_OBJECT (todo), t->vc ? t->vc : t->tag, value, NULL);
	  else if (t->type == G_TYPE_INT) 
	    {
	      if (!strcasecmp (t->tag, "STATE"))
		    {
              gint val = atol(value);
              switch (val)
              {
                case NOT_STARTED:
		          g_object_set (G_OBJECT (todo), t->vc ? t->vc : t->tag, MIMEDIR_STATUS_NEEDS_ACTION, NULL);
                break;
                case IN_PROGRESS:
		          g_object_set (G_OBJECT (todo), t->vc ? t->vc : t->tag, MIMEDIR_STATUS_IN_PROCESS, NULL);
                break;
                case ABANDONED:
                case COMPLETED:
                  // Completed Task
		          g_object_set (G_OBJECT (todo), t->vc ? t->vc : t->tag, MIMEDIR_STATUS_COMPLETED, NULL);
		          g_object_set (G_OBJECT (todo), "dtcompleted", mimedir_datetime_new_from_time_t (time(NULL)), NULL);
                break;
                default:
		          g_object_set (G_OBJECT (todo), t->vc ? t->vc : t->tag, MIMEDIR_STATUS_NEEDS_ACTION, NULL);
                break;
              }
            }
          else
            g_object_set (G_OBJECT (todo), t->vc ? t->vc : t->tag, atoi (value), NULL);
	    } 
	  else
	    return FALSE;

	  return TRUE;
	}
      t++;
    }

  if (!strcasecmp (tag, "due"))
    {
      struct tm tm;

      memset (&tm, 0, sizeof (tm));
      if (strptime (value, "%F", &tm))
	{
	  MIMEDirDateTime *date;

	  date = mimedir_datetime_new_from_struct_tm (&tm);

	  g_object_set (G_OBJECT (todo), "due", date, NULL);
	}

      return TRUE;
    }

  return FALSE;
}

MIMEDirVTodo *
vtodo_from_tags (GSList *tags)
{
  MIMEDirVTodo *vtodo = mimedir_vtodo_new ();

  while (tags)
    {
      gpe_tag_pair *p = tags->data;

      vtodo_interpret_tag (vtodo, p->tag, p->value);

      tags = tags->next;
    }

  return vtodo;
}

GSList *
vtodo_to_tags (MIMEDirVTodo *vtodo)
{
  GSList *data = NULL;
  struct tag_map *t = &map[0];

  while (t->tag)
    {
      if (t->type == G_TYPE_STRING)
	{
	  gchar *value;

	  g_object_get (G_OBJECT (vtodo), t->vc ? t->vc : t->tag, &value, NULL);

	  if (value)
	    data = gpe_tag_list_prepend (data, t->tag, g_strstrip (value));
	}
      else if (t->type == G_TYPE_INT)
	{
	  gint value;

	  g_object_get (G_OBJECT (vtodo), t->vc ? t->vc : t->tag, &value, NULL);

	  // Convert from MIMEDir's Status to GPE's state
	  if (! strcasecmp (t->tag, "STATE"))
        {
          switch (value)
            {
              case MIMEDIR_STATUS_COMPLETED:
                value = COMPLETED;
              break;
              case MIMEDIR_STATUS_IN_PROCESS:
                value = IN_PROGRESS;
              break;
              default:
                value = NOT_STARTED;
              break;
            }
        }
	  // The default priority of 0 seems to annoy GPE-Todo?
	  if (! strcasecmp (t->tag, "PRIORITY"))
	    value = (value >= 5) ? value : 5;
	  data = gpe_tag_list_prepend (data, t->tag, g_strdup_printf ("%d", value));
	}
      else
	abort ();

      t++;
    }

  return data;
}
