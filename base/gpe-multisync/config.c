/* 
 * MultiSync GPE Plugin
 * Copyright (C) 2004 Phil Blundell <pb@nexus.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 */

#include <stdlib.h>
#include <errno.h>
#include <glib.h>
#include <libintl.h>
#include <stdio.h>
#include <string.h>

#include "multisync.h"

#include "gpe_sync.h"

gchar *
gpe_config_path (gpe_conn *conn)
{
  gchar *filename;

  filename = g_strdup_printf ("%s/%s", 
			      sync_get_datapath (conn->sync_pair),
			      "gpe_config.dat");

  return filename;
}

gboolean
gpe_load_config (gpe_conn *conn)
{
  gchar *path;
  FILE *fp;

  path = gpe_config_path (conn);

  fp = fopen (path, "r");
  if (fp)
    {
      char buf[256];
      
      if (fgets (buf, sizeof (buf), fp))
	{
	  buf [strlen (buf) - 1] = 0;
	  conn->device_addr = g_strdup (buf);
	}

      if (fgets (buf, sizeof (buf), fp))
	{
	  buf [strlen (buf) - 1] = 0;
	  conn->username = g_strdup (buf);
	}

      fclose (fp);
    }
  else
    {
      conn->username = g_strdup (g_get_user_name ());
      conn->device_addr = g_strdup ("localhost");
    }

  g_free (path);

  return TRUE;
}

gboolean
gpe_save_config (gpe_conn *conn)
{
  gchar *path;
  FILE *fp;

  path = gpe_config_path (conn);

  fprintf (stderr, "Saving config to %s\n", path);

  fp = fopen (path, "w");
  if (fp)
    {
      fprintf (fp, "%s\n", conn->device_addr);
      fprintf (fp, "%s\n", conn->username);
      fclose (fp);
    }

  g_free (path);

  return TRUE;
}
