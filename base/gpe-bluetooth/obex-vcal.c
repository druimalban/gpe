/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
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

#include <gpe/vevent.h>

#include <sqlite.h>

#include "obexserver.h"

#define _(x)  (x)

static void
do_import_vcal (MIMEDirVCal *cal)
{
}

void
import_vcal (const gchar *data, size_t len)
{
  MIMEDirVCal *cal;
  gchar *str;
  GError *error = NULL;

  str = g_malloc (len + 1);
  memcpy (str, data, len);
  str[len] = 0;

  cal = mimedir_vcal_new_from_string (str, &error);
 
  g_free (str);

  if (cal)
    {
      gchar *query;

      query = g_strdup_printf (_("Received a calendar entry.  Import it?"));

      if (gpe_question_ask (query, NULL, "bt-logo", "!gtk-cancel", NULL, "!gtk-ok", NULL, NULL))
	do_import_vcal (cal);

      g_object_unref (cal);
    }
}

