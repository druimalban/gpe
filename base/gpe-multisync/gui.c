/* 
 * MultiSync GPE Plugin
 * Copyright (C) 2004 Phil Blundell <pb@nexus.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 */

#include <gtk/gtk.h>
#include <glade/glade.h>

#include "multisync.h"

GtkWidget *config_window;

gboolean
create_config_window (void)
{
  GladeXML *xml;
  gchar *filename;

  filename = g_build_filename (PREFIX, "share", PACKAGE,
			       "gpe_sync.glade", NULL);

  xml = glade_xml_new (filename, NULL, NULL);

  g_free (filename);

  if (xml == NULL)
    return FALSE;

  config_window = glade_xml_get_widget (xml, "gpe_config");

  return TRUE;
}

GtkWidget* 
open_option_window (sync_pair *pair, connection_type type)
{
  if (! config_window)
    {
      if (create_config_window () == FALSE)
	return NULL;
    }

  return config_window;
}
