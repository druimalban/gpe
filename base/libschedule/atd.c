/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "gpe/schedule.h"

#define ATD_BASE "/var/spool/at"

#define TRIGGER ATD_BASE "/trigger"

static gboolean
trigger_atd (void)
{
  int fd = open (TRIGGER, O_WRONLY);
  gboolean rc = TRUE;
  if (fd < 0)
    {
      perror (TRIGGER);
      return FALSE;
    }
  
  if (write (fd, "\n", 1) < 0)
    rc = FALSE;
  if (close (fd) < 0)
    rc = FALSE;

  return rc;
}

static gchar *
alarm_filename (guint id, time_t start)
{
  uid_t uid = getuid ();
  return g_strdup_printf ("%s/%ld.%ld-%ld", ATD_BASE, start, id, uid);
}

static const char *boilerplate_1 = "#!/bin/sh\nrm -f $0\nDISPLAY=:0\nexec ";

gboolean
schedule_set_alarm (guint id, time_t start, const gchar *action)
{
  int fd;
  gchar *filename = alarm_filename (id, start);

  fd = open (filename, O_WRONLY | O_CREAT, 0700);
  if (fd < 0)
    {
      perror (filename);
      g_free (filename);
      return FALSE;
    }

  write (fd, boilerplate_1, strlen (boilerplate_1));
  write (fd, action, strlen (action));

  close (fd);

  g_free (filename);

  return trigger_atd ();
}

gboolean
schedule_cancel_alarm (guint id, time_t start)
{
  gchar *filename = alarm_filename (id, start);

  if (unlink (filename))
    {
      perror (filename);
      g_free (filename);
      return FALSE;
    }

  g_free (filename);

  return trigger_atd ();
}

