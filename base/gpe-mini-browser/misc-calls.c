/*
 * gpe-mini-browser v0.16
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

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <webi.h>

#include <gdk/gdk.h>

#include <glib.h>
#include <gpe/init.h>
#include <gpe/errorbox.h>

#include "gpe-mini-browser.h"

//#define DEBUG /* uncomment this if you want debug output*/

void set_default_settings(Webi *html, WebiSettings *ks)
{
  const gchar* http_proxy;

  ks->default_font_size = 11;
  ks->default_fixed_font_size = 11;
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
  if (!http_proxy) {
    http_proxy = "" ;
  }
  ks->http_proxy = g_strdup(http_proxy);

  webi_set_settings (WEBI (html), ks);
}

void zoom_in (GtkWidget *zoom_in , gpointer *data)
{
  struct zoom_data *zoom;
  WebiSettings *set;

  zoom = (struct zoom_data *)data;
  set = zoom->settings;
  set->default_font_size++;
  set->default_fixed_font_size++;
  webi_set_settings (WEBI(zoom->html), set);
}

void zoom_out (GtkWidget *zoom_out , gpointer *data)
{
  struct zoom_data *zoom;
  WebiSettings *set;

  zoom = (struct zoom_data *)data;
  set = zoom->settings;
  set->default_font_size--;
  set->default_fixed_font_size--;
  webi_set_settings (WEBI(zoom->html), set);
}


