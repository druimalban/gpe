/*
 * gpe-mini-browser v0.19
 *
 * Basic web browser based on gtk-webcore 
 * 
 * Page loading calls.
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

#include <gdk/gdk.h>

#include <glib.h>
#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/gpedialog.h>

#include "gpe-mini-browser.h"

//#define DEBUG /* uncomment this if you want debug output*/

/* read a file or url into gtk-webcore */
void
fetch_url (const gchar * url, GtkWidget * html)
{
  gboolean file = FALSE;
  gchar *cut;

#ifdef DEBUG
  fprintf (stderr, "url is %s\n", url);
#endif

  file = g_str_has_prefix (url, "file");
  if (file)
    {
      cut = strchr (url, ':');
      if (cut)
	{
	  cut = cut + 3;
	}
      if (access (cut, F_OK))
	{
#ifdef DEBUG
	  fprintf (stderr, "Could not open %s\n", url);
#endif
	  file = TRUE;
	  gpe_error_box (_("Could not open file"));
	}
      else
	{
	  file = FALSE;
	}
    }

  if (!strcmp (url, "about:"))
    {
      gpe_info_dialog (_("GPE mini-browser v0.19\n\nHTML engine : Gtk-webcore \n(http://gtk-webcore.sourceforge.net)\n\nCopyright (c) Philippe De Swert\n<philippedeswert@scarlet.be>\n"));
    }


  if (!file)
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
      return url;
    }
  else
    {
      p = g_strconcat ("http://", url, NULL);
    }
  return (p);
}

void
load_text_entry (GtkWidget * Button, gpointer * text)
{
  struct url_data *data;
  const gchar *url;
  GtkTreeIter iter;

  data = (struct url_data *) text;
  webi_stop_load (WEBI (data->html));

  url = gtk_editable_get_chars (GTK_EDITABLE (data->entry), 0, -1);
#ifdef DEBUG
  printf ("url = %s\n", url);
#endif
  if (url != NULL)
    {
      /* add to list before url is parsed to avoid the user having to use http:// for autocompletion */
      gtk_list_store_insert (completion_store, &iter, 0);
      gtk_list_store_set (completion_store, &iter, 0, url, -1);
      url = parse_url (url);
#ifdef DEBUG
      printf ("fetching %s !\n", url);
#endif
      fetch_url (url, data->html);
    }
}


void
handle_cookie (Webi * html, WebiCookie * cookie, gpointer * data)
{
#ifdef DEBUG
  printf ("Site %s wants to set a cookie.\n", cookie->domain);
#endif
  /* accept all cookies by default for the moment */
  cookie->out_accept_cookie = TRUE;
}

/* makes the engine go forward one page */
void
forward_func (GtkWidget * forward, GtkWidget * html)
{
  if (webi_can_go_forward (WEBI (html)))
    webi_go_forward (WEBI (html));
  else
    gpe_error_box ("no more pages forward!");

}

/* makes the engine go back one page */
void
back_func (GtkWidget * back, GtkWidget * html)
{
  if (webi_can_go_back (WEBI (html)))
    webi_go_back (WEBI (html));
  else
    gpe_error_box ("No more pages back!");


}

/* makes the engine load the home page, if none exists default to gpe.handhelds.org :-) */
void
home_func (GtkWidget * home, GtkWidget * html)
{
  char *homepage = NULL;

  homepage = malloc (sizeof (char) * 60);
  strncpy (homepage, "gpe.handhelds.org", 18);

  start_db ();

  if (!get_bookmark_home (homepage))
    {
      homepage = (char *) parse_url (homepage);
      fetch_url (homepage, GTK_WIDGET (html));
    }
  else
    {
      homepage = (char *) parse_url (homepage);
      fetch_url (homepage, GTK_WIDGET (html));
    }

  stop_db ();
  free (homepage);

}

/* tell the engine to stop or reload the current page */
void
stop_reload_func (GtkWidget * reload, GtkWidget * html)
{
  const gchar *id;

  id = gtk_tool_button_get_stock_id (GTK_TOOL_BUTTON (stop_reload_button));
  if (!strcmp (id, "gtk-refresh"))
    webi_refresh (WEBI (html));
  else
    webi_stop_load (WEBI (html));
}
