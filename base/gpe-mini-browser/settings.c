/*
 * gpe-mini-browser v0.19
 *
 * Basic web browser based on gtk-webcore 
 * 
 * Misc calls.
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
#include <stddef.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <webi.h>


#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gdk/gdk.h>

#include <glib.h>
#include <glib/gkeyfile.h>
#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/question.h>
#include <gpe/gpedialog.h>
#include <gpe/picturebutton.h>

#include "gpe-mini-browser.h"

#ifdef HILDON
/* Hildon includes */
#include <hildon-widgets/hildon-app.h>
#include <hildon-widgets/hildon-appview.h>
#include <libosso.h>
#endif /*HILDON*/

//#define DEBUG /* uncomment this if you want debug output*/

void
set_default_settings (Webi * html, WebiSettings * ks)
{
  const gchar *http_proxy;
#ifndef HILDON
  ks->default_font_size = 11;
  ks->default_fixed_font_size = 11;
#else
  ks->default_font_size = 14;
  ks->default_fixed_font_size = 14;
#endif
  ks->minimum_font_size = 7;
  ks->serif_font_family = "serif";
  ks->sans_serif_font_family = "sans";
  ks->fixed_font_family = "sans";
  ks->standard_font_family = "sans";
  ks->autoload_images = 1;
  ks->javascript_enabled = 1;

  http_proxy = g_getenv ("http_proxy");
  if (!http_proxy)
    http_proxy = g_getenv ("HTTP_PROXY");
  if (!http_proxy)
    {
      http_proxy = "";
    }
  ks->http_proxy = g_strdup (http_proxy);

  webi_set_settings (WEBI (html), ks);
}

/*==============================================*/

void 
set_settings_from_file (Webi * html, WebiSettings * ks)
{
 
}

/*==============================================*/

void
zoom_in (GtkWidget * zoom_in, gpointer * data)
{
  struct zoom_data *zoom;
  WebiSettings *set;

  zoom = (struct zoom_data *) data;
  set = zoom->settings;
  set->default_font_size++;
  set->default_fixed_font_size++;
  webi_set_settings (WEBI (zoom->html), set);
}

/*==============================================*/

void
zoom_out (GtkWidget * zoom_out, gpointer * data)
{
  struct zoom_data *zoom;
  WebiSettings *set;

  zoom = (struct zoom_data *) data;
  set = zoom->settings;
  set->default_font_size--;
  set->default_fixed_font_size--;
  webi_set_settings (WEBI (zoom->html), set);
}

/*==============================================*/

int							
parse_settings_file(WebiSettings *ks)
{
  return(0);
}

/*==============================================*/

int 
write_settings_to_file(WebiSettings *ks)
{
  GKeyFile *settingsfile;
  gchar *output;
  FILE *outfile;
  char *filename = NULL;
  const char *home = getenv ("HOME");

  /* check if we can get the users home dir and create the file */
  if (home != NULL)
  {
    size_t len = strlen(home) + strlen(CONF_NAME);
    filename = malloc(len+1);
    strncpy(filename, home, len);
    strcat(filename, CONF_NAME); 
  }
  else
  {
   gpe_error_box(_("Could not save settings! Make sure HOME is set in your environment"));    
   free(filename);
   return(1);
  }
  outfile = fopen(filename, "w+");
  if(outfile <= 0)
  {
   gpe_error_box(_("Could not write file!"));
   free(filename);
   return(1);
  }

  settingsfile = g_key_file_new();
  g_key_file_set_string(settingsfile, "Config", "Proxy", ks->http_proxy);
  g_key_file_set_integer(settingsfile, "Config", "AutoImageLoading", ks->default_font_size);
  g_key_file_set_integer(settingsfile, "Config", "Javascript", ks->javascript_enabled);
  g_key_file_set_integer(settingsfile, "Config", "fontsize", ks->default_font_size);

  output = g_key_file_to_data(settingsfile,NULL, NULL);
  fprintf(outfile, output);
  fclose(outfile);
  free(filename);
  g_key_file_free(settingsfile);
  return(0);
}

/*==============================================*/

void
save_settings_on_quit(GtkWidget *app, WebiSettings *ks)
{
  write_settings_to_file(ks);
}
