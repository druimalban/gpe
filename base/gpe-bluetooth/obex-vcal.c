/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *               2007 Florian Boor <florian@linuxtogo.org>
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

#include <gtk/gtk.h>
#include <gpe/errorbox.h>
#include <gpe/question.h>

#include <openobex/obex.h>

#include <mimedir/mimedir-vcal.h>

#include <gpe/vtype.h>

#include "obexserver.h"

#define _(x)  (x)


static void
do_import_vcal (MIMEDirVCal *vcal)
{
  GSList *list, *iter;

  list = mimedir_vcal_get_event_list (vcal);

  for (iter = list; iter; iter = iter->next)
    {
      MIMEDirVEvent *vevent;
      vevent = MIMEDIR_VEVENT (list->data);
      event_import_from_vevent (NULL, vevent, NULL, NULL);
    }

  g_slist_free (list);

  list = mimedir_vcal_get_todo_list (vcal);

  for (iter = list; iter; iter = iter->next)
    {
      MIMEDirVTodo *vtodo;
      vtodo = MIMEDIR_VTODO (list->data);
      todo_import_from_vtodo (vtodo, NULL);
    }

  g_slist_free (list);
}

void
import_vcal (const gchar *data, size_t len)
{
  MIMEDirProfile *profile;
  MIMEDirVCal *cal = NULL;
  gchar *str;
  GError *error = NULL;

  str = g_malloc (len + 1);
  memcpy (str, data, len);
  str[len] = 0;

  profile = mimedir_profile_new(NULL);
  mimedir_profile_parse(profile, str, &error);
  if (!error)
    cal = mimedir_vcal_new_from_profile (profile, &error);
 
  g_free (str);

  if (cal)
    {
      gchar *query;

      query = g_strdup_printf (_("Received a calendar entry.  Import it?"));

      if (gpe_question_ask (query, NULL, "bt-logo", "!gtk-cancel", NULL, "!gtk-ok", NULL, NULL))
	    do_import_vcal (cal);

      g_object_unref (cal);
      g_object_unref (profile);
    }
  else
    {
      if (error)
        g_error_free(error);
    }  
}
