/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>

#include <glib.h>

#include "xsettings-common.h"
#include "xsettings-client.h"

Display *dpy;
XSettingsClient *client;

static void
print_xsetting (XSettingsSetting *setting)
{
  printf ("%s ", setting->name);

  switch (setting->type)
    {
    case XSETTINGS_TYPE_INT:
      printf ("int %d", setting->data.v_int);
      break;

    case XSETTINGS_TYPE_COLOR:
      printf ("col %04x %04x %04x %04x",
	      setting->data.v_color.red,
	      setting->data.v_color.green,
	      setting->data.v_color.blue,
	      setting->data.v_color.alpha);
      break;

    case XSETTINGS_TYPE_STRING:
      printf ("str %s", setting->data.v_string);
      break;
    }

  printf ("\n");
}


gboolean
xsetting_unstringize (char *s, XSettingsSetting *setting)
{
  int red, green, blue, alpha;

  if (s[0] == 0 || s[1] != ':')
    return FALSE;

  switch (s[0])
    {
    case 'C':
      setting->type = XSETTINGS_TYPE_COLOR;
      sscanf (s + 2, "%04x %04x %04x %04x",
	      &red,
	      &green,
	      &blue,
	      &alpha);
      setting->data.v_color.red = red;
      setting->data.v_color.green = green;
      setting->data.v_color.blue = blue;
      setting->data.v_color.alpha = alpha;
      break;

    case 'I':
      setting->type = XSETTINGS_TYPE_INT;
      setting->data.v_int = atoi (s + 2);
      break;

    case 'S':
      setting->type = XSETTINGS_TYPE_STRING;
      setting->data.v_string = g_strdup (s + 2);
      break;

    default:
      return FALSE;
    }

  return TRUE;
}

static void
notify_func (const char       *name,
	     XSettingsAction   action,
	     XSettingsSetting *setting,
	     void             *cb_data)
{
  if (action == XSETTINGS_ACTION_NEW)
    print_xsetting (setting);
}

static void
usage (void)
{
  fprintf (stderr, "Usage: xst read   [property]\n");
  fprintf (stderr, "       xst write  <property> <format> <value>\n");
  fprintf (stderr, "       xst delete <property>\n");
}

int
main (int argc, char *argv[])
{
  char *disp = NULL;
  gboolean flag_read = FALSE, flag_write = FALSE, flag_delete = FALSE;
  gchar *prop = NULL, *format = NULL, *value = NULL;
  XSettingsNotifyFunc notify_fp = NULL;

  if ((dpy = XOpenDisplay (disp)) == NULL)
    {
      fprintf (stderr, "Cannot connect to X server on display %s.\n",
	      XDisplayName (disp));
      exit (1);
    }

  if (argc < 2)
    {
      usage ();
      exit (1);
    }

  if (!strcmp (argv[1], "read"))
    {
      flag_read = TRUE;
      if (argc == 3)
	prop = argv[2];
    }
  else if (!strcmp (argv[1], "write"))
    {
      flag_write = TRUE;
      if (argc < 5)
	{
	  usage ();
	  exit (1);
	}
      prop = argv[2];
      format = argv[3];
      value = argv[4];
    }
  else if (!strcmp (argv[1], "delete"))
    {
      flag_delete = TRUE;
      if (argc < 3)
	{
	  usage ();
	  exit (1);
	}
      prop = argv[2];
    }
  else
    {
      usage ();
      exit (1);
    }

  if (flag_read)
    {
      if (prop == NULL)
	notify_fp = notify_func;

      client = xsettings_client_new (dpy, DefaultScreen (dpy), notify_fp, NULL, NULL);
      if (client == NULL)
	{
	  fprintf (stderr, "Cannot create XSettings client\n");
	  exit (1);
	}

      if (prop)
	{
	  XSettingsSetting *setting;
      
	  if (xsettings_client_get_setting (client, prop, &setting) == XSETTINGS_SUCCESS)
	    print_xsetting (setting);
	}

      xsettings_client_destroy (client);
    }
      
  if (flag_write || flag_delete)
    {
      Atom gpe_settings_update_atom = XInternAtom (dpy, "_GPE_SETTINGS_UPDATE", 0);
      Window manager = XGetSelectionOwner (dpy, gpe_settings_update_atom);
      XSettingsType type;
      size_t length, name_len;
      gchar *buffer;
      XClientMessageEvent ev;
      gboolean done = FALSE;
      Window win;

      if (manager == None)
	{
	  fprintf (stderr, "gpe-confd not running.\n");
	  exit (1);
	}

      win = XCreateSimpleWindow (dpy, DefaultRootWindow (dpy), 1, 1, 1, 1, 0, 0, 0);

      if (flag_write)
	{
	  if (!strcmp (format, "int"))
	    {
	      type = XSETTINGS_TYPE_INT;
	      length = 4;
	    }
	  else if (!strcmp (format, "col"))
	    {
	      type = XSETTINGS_TYPE_COLOR;
	      length = 8;
	    }
	  else if (!strcmp (format, "str"))
	    {
	      type = XSETTINGS_TYPE_STRING;
	      length = 4 + ((strlen (value) + 3) & ~3);
	    }
	  else
	    {
	      fprintf (stderr, "Format \"%s\" not recognised (try int/col/str)\n", format);
	      exit (1);
	    }
	}
      else
	length = 0;
      
      name_len = strlen (prop);
      name_len = (name_len + 3) & ~3;
      buffer = g_malloc (length + 4 + name_len);
      if (flag_write)
	*buffer = type;
      else
	*buffer = 0xff;
      buffer[1] = 0;
      buffer[2] = name_len & 0xff;
      buffer[3] = (name_len >> 8) & 0xff;
      memcpy (buffer + 4, prop, name_len);

      if (flag_write)
	{
	  switch (type)
	    {
	    case XSETTINGS_TYPE_INT:
	      *((unsigned long *)(buffer + 4 + name_len)) = atoi (value);
	      break;
	    case XSETTINGS_TYPE_STRING:
	      *((unsigned long *)(buffer + 4 + name_len)) = strlen (value);
	      memcpy (buffer + 8 + name_len, value, strlen (value));
	      break;
	    case XSETTINGS_TYPE_COLOR:
	      break;
	    }
	}
      
      XChangeProperty (dpy, win, gpe_settings_update_atom, gpe_settings_update_atom,
		       8, PropModeReplace, buffer, length + 4 + name_len);
      
      g_free (buffer);

      XSelectInput (dpy, win, PropertyChangeMask);
      
      ev.type = ClientMessage;
      ev.window = win;
      ev.message_type = gpe_settings_update_atom;
      ev.format = 32;
      ev.data.l[0] = gpe_settings_update_atom;
      XSendEvent (dpy, manager, FALSE, 0, (XEvent *)&ev);

      while (! done)
	{
	  XEvent ev;

	  XNextEvent (dpy, &ev);

	  switch (ev.xany.type)
	    {
	    case PropertyNotify:
	      if (ev.xproperty.window == win
		  && ev.xproperty.atom == gpe_settings_update_atom)
		done = TRUE;
	      break;
	    }
	}
    }

  XCloseDisplay (dpy);
  
  exit (0);
}
