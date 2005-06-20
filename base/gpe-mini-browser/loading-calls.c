/*
 * gpe-mini-browser v0.1
 *
 * Basic web browser based on gtk-webcore 
 * 
 * Interface calls.
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

#define HOME_PAGE "file:///usr/share/doc/gpe/mini-browser-index.html"

#define DEBUG /* uncomment this if you want debug output*/

/* read a file or url into gtk-webcore */
void
fetch_url (const gchar * url, GtkWidget * html)
{
    gboolean file=FALSE;	
    gchar *cut;

    file = g_str_has_prefix(url, "file");
    if(file)
    {
     cut = strchr (url, ':');
     if (cut)
	{
	 cut = cut+3;
	}
     if(access(cut, F_OK))
        {
#ifdef DEBUG
         fprintf (stderr, "Could not open %s\n", url);
#endif
         gpe_error_box("Could not open file");
        }	
     else
	{
	file = FALSE;
	}	
    }

    if(!file)	
    {
      webi_stop_load (WEBI (html));
      webi_load_url (WEBI (html), url);
    }
}

/* parse url to something easy to interprete */
const gchar *
parse_url (const gchar * url)
{
  const gchar *p;


  p = strchr (url, ':');
  if (p)
    {
      return p;
    }
  else
    {
      p = g_strconcat ("http://", url, NULL);
    }
  return (p);
}

void load_text_entry (GtkWidget * Button, gpointer * text)
{
	struct url_data *data;
	gchar *url;
	
        data =  (struct url_data *)text;

	url = gtk_editable_get_chars (GTK_EDITABLE (data->entry), 0, -1);
#ifdef DEBUG
	printf("url = %s\n", url);
#endif
	if(url != NULL)	
	{
		url = parse_url(url);
#ifdef DEBUG
		printf("fetching %s !\n", url);
#endif
		fetch_url(url, data->html);
	}
}

