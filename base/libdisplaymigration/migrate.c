/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <ctype.h>
#include <libintl.h>
#include <string.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include "auth.h"
#include "libdm.h"

#define _(x) gettext(x)

static GdkAtom string_gdkatom, display_change_gdkatom;
static GdkAtom rsa_challenge_gdkatom;

#define DISPLAY_CHANGE_SUCCESS			0
#define DISPLAY_CHANGE_UNABLE_TO_CONNECT	1
#define DISPLAY_CHANGE_NO_SUCH_SCREEN		2
#define DISPLAY_CHANGE_AUTHENTICATION_BAD	3
#define DISPLAY_CHANGE_INDETERMINATE_ERROR	4

static gboolean no_auth;

static GSList *all_widgets;

static gboolean libdm_initialised;

static int
do_change_display (GtkWidget *w, char *display_name)
{
  GdkDisplay *newdisplay;
  guint screen_nr = 1;
  guint i;

  if (display_name[0] == 0)
    return DISPLAY_CHANGE_INDETERMINATE_ERROR;

  i = strlen (display_name) - 1;
  while (i > 0 && isdigit (display_name[i]))
    i--;

  if (display_name[i] == '.')
    {
      screen_nr = atoi (display_name + i + 1);
      display_name[i] = 0;
    }

  newdisplay = gdk_display_open (display_name);
  if (newdisplay)
    {
      GdkScreen *screen = gdk_display_get_screen (newdisplay, screen_nr);
      if (screen)
	{
	  gtk_window_set_screen (GTK_WINDOW (w), screen);
	  gdk_display_manager_set_default_display (gdk_display_manager_get (),
						   newdisplay);
	  return DISPLAY_CHANGE_SUCCESS;
	}
      else
	return DISPLAY_CHANGE_NO_SUCH_SCREEN;
    }

  return DISPLAY_CHANGE_UNABLE_TO_CONNECT;
}

static void
set_challenge_on_window (GdkWindow *window)
{
  gdk_property_change (window, rsa_challenge_gdkatom, string_gdkatom,
		       8, GDK_PROP_MODE_REPLACE, libdm_auth_challenge_string, 
		       strlen (libdm_auth_challenge_string));
}

static void
update_challenge_on_windows (void)
{
  GSList *i;

  libdm_auth_update_challenge ();

  for (i = all_widgets; i; i = i->next)
    {
      GtkWidget *w = GTK_WIDGET (i->data);
      if (w->window)
	set_challenge_on_window (w->window);
    }
}

static void
reset_state (GdkWindow *window)
{
  gdk_property_change (window, display_change_gdkatom, string_gdkatom,
		       8, GDK_PROP_MODE_REPLACE, NULL, 0);
}

static void
generate_response (GdkDisplay *gdisplay, Display *dpy, Window window, int code)
{
  XClientMessageEvent ev;
  Atom atom = gdk_x11_atom_to_xatom_for_display (gdisplay, 
						 display_change_gdkatom);
  
  memset (&ev, 0, sizeof (ev));
  
  ev.type = ClientMessage;
  ev.window = window;
  ev.message_type = atom;
  ev.format = 32;
  
  ev.data.l[0] = window;
  ev.data.l[1] = code;
  
  XSendEvent (dpy, DefaultRootWindow (dpy), False, 
	      SubstructureNotifyMask, (XEvent *)&ev);
}

static int
handle_request (GdkWindow *gwindow, char *prop)
{
  GtkWidget *widget;
  char *target, *auth_method, *auth_data;
  char *p;

  target = prop;
  auth_method = "NULL";
  auth_data = NULL;

  p = strchr (prop, ' ');
  if (p)
    {
      *p = 0;
      auth_method = ++p;
      
      p = strchr (p, ' ');
      if (p)
	{
	  *p = 0;
	  auth_data = ++p;
	}
    }

  if (no_auth == FALSE)
    {
      if (!strcasecmp (auth_method, "null"))
	return DISPLAY_CHANGE_AUTHENTICATION_BAD;
      else if (!strcasecmp (auth_method, "rsa-sig"))
	{
	  if (libdm_auth_validate_request (target, auth_data) == FALSE)
	    return DISPLAY_CHANGE_AUTHENTICATION_BAD;
	}
      else
	return DISPLAY_CHANGE_AUTHENTICATION_BAD;
    }

  gdk_window_get_user_data (gwindow, (gpointer*) &widget);

  if (widget)
    return do_change_display (widget, target);

  return DISPLAY_CHANGE_INDETERMINATE_ERROR;
}

static GdkFilterReturn 
filter_func (GdkXEvent *xevp, GdkEvent *ev, gpointer p)
{
  XPropertyEvent *xev = (XPropertyEvent *)xevp;

  if (xev->type == PropertyNotify)
    {
      GdkDisplay *gdisplay;
      Atom atom;

      gdisplay = gdk_x11_lookup_xdisplay (xev->display);
      if (gdisplay)
	{
	  atom = gdk_x11_atom_to_xatom_for_display (gdisplay, display_change_gdkatom);

	  if (xev->atom == atom)
	    {
	      GdkWindow *gwindow;
	      
	      gwindow = gdk_window_lookup_for_display (gdisplay, xev->window);
	      
	      if (gwindow)
		{
		  GdkAtom actual_type;
		  gint actual_format;
		  gint actual_length;
		  unsigned char *prop = NULL;
		  
		  if (gdk_property_get (gwindow, display_change_gdkatom, string_gdkatom,
					0, G_MAXLONG, FALSE, &actual_type, &actual_format,
					&actual_length, &prop))
		    {
		      if (actual_length != 0)
			{
			  if (actual_type == string_gdkatom && actual_length > 8)
			    {
			      gchar *buf = g_malloc (actual_length + 1);
			      int rc;
			      
			      memcpy (buf, prop, actual_length);
			      buf[actual_length] = 0;
			      
			      rc = handle_request (gwindow, buf);
			      
			      g_free (buf);
			      generate_response (gdisplay, xev->display, xev->window, rc);
			      
			      if (rc == DISPLAY_CHANGE_SUCCESS)
				update_challenge_on_windows ();
			    }
			  
			  reset_state (gwindow);
			}
		    }
		  
		  if (prop)
		    g_free (prop);
		}
	    }
	  
	  return GDK_FILTER_REMOVE;
	}
    }

  return GDK_FILTER_CONTINUE;
}

static void
unrealize_window (GtkWidget *w)
{
  all_widgets = g_slist_remove (all_widgets, w);
}

void
libdm_mark_window (GtkWidget *w)
{
  if (! libdm_initialised)
    {
      g_warning ("libdm not initialised yet");
      return;
    }

  if (GTK_WIDGET_REALIZED (w))
    {
      GdkWindow *window = w->window;
      
      gdk_window_add_filter (window, filter_func, NULL);

      reset_state (window);
      set_challenge_on_window (window);

      all_widgets = g_slist_append (all_widgets, w);

      g_signal_connect (G_OBJECT (w), "unrealize", G_CALLBACK (unrealize_window), NULL);
    }
  else
    g_signal_connect (G_OBJECT (w), "realize", G_CALLBACK (libdm_mark_window), NULL);
}

void
libdm_init (void)
{
  if (getenv ("GPE_DISPLAY_MIGRATION_NO_AUTH") != NULL)
    no_auth = TRUE;

  string_gdkatom = gdk_atom_intern ("STRING", FALSE);
  display_change_gdkatom = gdk_atom_intern ("_GPE_DISPLAY_CHANGE", FALSE);
  rsa_challenge_gdkatom = gdk_atom_intern ("_GPE_DISPLAY_CHANGE_RSA_CHALLENGE", FALSE);

  libdm_auth_generate_challenge ();

  libdm_initialised = TRUE;
}
