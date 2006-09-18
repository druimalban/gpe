/*
 * gpe-helpindex v0.1 (for use with gpe-helpviewer)
 *
 * Generates HTML help index file from a configuration file. 
 * With the following structure : 
 * [Help]
 * application-name=full path to application help
 *
 * Copyright (c) 2005 Philippe De Swert
 *
 * Contact : philippedeswert@scarlet.be
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gtk/gtk.h>

#include <glib.h>
#include <glib/gkeyfile.h>

int
main (int argc, char *argv[])
{
  GKeyFile *config;		/* stockage for GKeyfile in memory */
  gchar *value, **keys;		/* store keys and values for the keys */
  gboolean test;
  FILE *html;
  const char *filename = "/usr/share/doc/gpe/help-index.html";

  /* create memory space for GKeyfile and let config point to it */
  config = g_key_file_new ();
  /* open (or create) the help index html file */
  html = fopen (filename, "w+");
  if (html <= 0)
    {
      printf ("file could not be opened or created!\n");
      g_key_file_free(config);
      exit (1);
    }
  if (argc == 2)
    {
      test =
	g_key_file_load_from_file (config, argv[1], G_KEY_FILE_NONE, NULL);
    }
  else
    {
      test =
	g_key_file_load_from_file (config, "/etc/gpe/gpe-help.conf",
				   G_KEY_FILE_NONE, NULL);
    }
  /* if test == true then the config file has succesfully been parsed and imported */
  if (test)
    {
      fprintf (html, "<HTML><H2>Application help index</H2>\n");
      /* get the keys */
      keys = g_key_file_get_keys (config, "Help", NULL, NULL);
      /* loop through the keys to get their values and print them to the screen and the file */
      while (*keys != NULL)
	{
	  value = g_key_file_get_value (config, "Help", *keys, NULL);
	  printf ("key is: %s with value %s\n", *keys, value);
	  fprintf (html, "<A HREF=\"%s\">%s</a><BR>\n", value, *keys);
	  ++keys;
	}
      fprintf (html, "</HTML>");
      fclose (html);
      g_key_file_free(config);
      exit(0);
    }
  else
    {
      printf ("PARSE ERROR : please specify the  correct config file on the command line!\n");
      g_key_file_free(config);
      exit(1);
    }
}
