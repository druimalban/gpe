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
#include <sys/stat.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>

#include <glib.h>

#include "xsettings-manager.h"

XSettingsManager *manager;

#ifdef GPE_CONF_DB
#include <db.h>
DB *database;
#else
#include <sqlite3.h>
sqlite3 *database;
/* define own sqlite3 equivalent of sqlite3_exec_printf */
#define sqlite3_exec_printf(handle_, query_, cb_, cookie_, err_, args_...) \
  ({ char *q_ = sqlite3_mprintf (query_ , ## args_); \
     int ret_ = sqlite3_exec (handle_, q_, cb_, cookie_, err_); \
     sqlite3_free (q_); \
     ret_; })
#endif

Atom gpe_settings_update_atom;
gboolean defaults_needed;

char *
xsetting_stringize (XSettingsSetting *setting)
{
  char *s = NULL;

  switch (setting->type)
    {
    case XSETTINGS_TYPE_INT:
      s = g_strdup_printf ("I:%d", setting->data.v_int);
      break;

    case XSETTINGS_TYPE_COLOR:
      s = g_strdup_printf ("C:%04x %04x %04x %04x",
			   setting->data.v_color.red,
			   setting->data.v_color.green,
			   setting->data.v_color.blue,
			   setting->data.v_color.alpha);
      break;

    case XSETTINGS_TYPE_STRING:
      s = g_strdup_printf ("S:%s", setting->data.v_string);
      break;

    case XSETTINGS_TYPE_NONE:
      break;
    }

  return s;
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

#ifdef GPE_CONF_DB

gboolean
suck_in_settings (XSettingsManager *manager, DB *db)
{
  DBC *cursor;
  DBT key, data;

  if (db->cursor (db, NULL, &cursor, 0))
    return FALSE;

  memset (&key, 0, sizeof (key));
  memset (&data, 0, sizeof (data));

  while (cursor->c_get (cursor, &key, &data, DB_NEXT) == 0)
    {
      XSettingsSetting setting;
      if (xsetting_unstringize (data.data, &setting))
	{
	  setting.name = g_strdup (key.data);
	  xsettings_manager_set_setting (manager, &setting);
	}
    }

  cursor->c_close (cursor);

  xsettings_manager_notify (manager);

  return TRUE;
}

void
db_store_setting (DB *db, XSettingsSetting *setting)
{
  DBT key, data;
  
  memset (&key, 0, sizeof (key));
  memset (&data, 0, sizeof (data));

  key.data = setting->name;
  key.size = strlen (setting->name) + 1;

  data.data = xsetting_stringize (setting);
  data.size = strlen (data.data) + 1;

  db->put (db, NULL, &key, &data, 0);

  free (data.data);
}

DB *
database_open (void)
{
  DB *db;
  char *home = g_get_home_dir (), *file;
  struct stat buf;

  file = g_strdup_printf ("%s/.gpe", home);
  if (stat (file, &buf) != 0)
    {
      if (mkdir (file, 0700) != 0)
	{
	  fprintf (stderr, "Cannot create ~/.gpe");
	  g_free (file);
	  return FALSE;
	}
    } 
  else 
    {
      if (!S_ISDIR (buf.st_mode))
	{
	  fprintf (stderr, "ERROR: ~/.gpe is not a directory!");
	  g_free (file);
	  return FALSE;
	}
    }

  g_free (file);
  
  file = g_strdup_printf ("%s/.gpe/settings.db", home);
 
  if (db_create (&db, NULL, 0))
    return NULL;

  if (db->open (db, file, NULL, DB_BTREE, DB_CREATE, 0600))
    return NULL;

  g_free (file);

  return db;
}

#else

static int
read_one_setting (void *arg, int argc, char **argv, char **names)
{
  if (argc == 2)
    {
      XSettingsSetting setting;
      if (xsetting_unstringize (argv[1], &setting))
	{
	  setting.name = g_strdup (argv[0]);
	  xsettings_manager_set_setting (manager, &setting);
	}
    }

  return 0;
}

void
db_store_setting (sqlite3 *db, XSettingsSetting *setting)
{
  gchar *str =  xsetting_stringize (setting);
  char *err;

  if (sqlite3_exec_printf (db, "delete from xsettings where key='%q'", NULL, NULL, &err,
			  setting->name))
    {
      fprintf (stderr, "sqlite: %s\n", err);
      free (err);
    }

  if (setting->type != 0xff)
    {
      if (sqlite3_exec_printf (db, "insert into xsettings values ('%q', '%q')", NULL, NULL, &err,
			      setting->name, str))
	{
	  fprintf (stderr, "sqlite: %s\n", err);
	  free (err);
	}
    }

  g_free (str);
}

void
load_defaults_file (XSettingsManager *manager, sqlite3 *db, const char *file)
{
  FILE *fp = fopen (file, "r");
  if (fp)
    {
      char buf[128];
      while (!feof (fp))
	{
	  if (fgets (buf, sizeof (buf), fp) && buf[0] != '#')
	    {
	      XSettingsSetting s;
	      char *p;
	      size_t l = strlen (buf);
	      if (l == 0 || l == 1)
		continue;
	      buf[l - 1] = 0;
	      p = strchr (buf, ':');
	      if (!p)
		{
		  fprintf (stderr, "bad line in defaults file: \"%s\"\n", buf);
		  continue;
		}
	      *(p++) = 0;
	      if (xsetting_unstringize (p, &s))
		{
		  s.name = g_strdup (buf);
		  db_store_setting (db, &s);
		  xsettings_manager_set_setting (manager, &s);
		}
	      else
		fprintf (stderr, "couldn't unstringize \"%s\"\n", p);
	    }
	}

      fclose (fp);
    }
}

#define DEFAULT_FILE "/etc/gpe/xsettings.default"
#define DEFAULT_DIR  "/etc/gpe/xsettings-default.d"

void
load_defaults (XSettingsManager *manager, sqlite3 *db)
{
  GDir *dir = g_dir_open (DEFAULT_DIR, 0, NULL);
  char *file = g_strdup (DEFAULT_FILE);
  const char *entry = NULL;

  while (file)
    {
      load_defaults_file (manager, db, file);
      g_free (file);
      file = NULL;

      if (dir)
	{
	  if ((entry = g_dir_read_name (dir)) != NULL)
	    file = g_build_filename (DEFAULT_DIR, entry, NULL);
	  else
	    g_dir_close (dir);
	}
    }
}

gboolean
suck_in_settings (XSettingsManager *manager, sqlite3 *db)
{
  char *err;

  if (sqlite3_exec (db, "select key, value from xsettings", read_one_setting,
		   NULL, &err))
    {
      fprintf (stderr, "sqlite: %s\n", err);
      free (err);
      return FALSE;
    }

  if (defaults_needed)
    load_defaults (manager, db);

  xsettings_manager_notify (manager);

  return TRUE;
}

sqlite3 *
database_open (void)
{
  sqlite3 *db;
  const char *home = g_get_home_dir ();
  char *err, *file;
  struct stat buf;

  file = g_strdup_printf ("%s/.gpe", home);
  if (stat (file, &buf) != 0)
    {
      if (mkdir (file, 0700) != 0)
	{
	  fprintf (stderr, "Cannot create ~/.gpe");
	  g_free (file);
	  return FALSE;
	}
    } 
  else 
    {
      if (!S_ISDIR (buf.st_mode))
	{
	  fprintf (stderr, "ERROR: ~/.gpe is not a directory!");
	  g_free (file);
	  return FALSE;
	}
    }

  g_free (file);

  file = g_strdup_printf ("%s/.gpe/settings", home);
 
  sqlite3_open (file, &db);
  g_free (file);

  if (db)
    {
      static const char *schema_info = "create table xsettings (key text, value text)";
      if (sqlite3_exec (db, schema_info, NULL, NULL, &err) == 0)
	defaults_needed = TRUE;
    }
  else
    {
      fprintf (stderr, "sqlite: %s\n", err);
      free (err);
    }

  return db;
}

#endif

void
write_setting (Display *dpy, Window w, unsigned long prop)
{
  XSettingsSetting setting;
  unsigned long length;
  unsigned long nitems;
  int actual_format;
  Atom actual_type;
  unsigned char *data, *p;
  unsigned int name_len, len;

  if (XGetWindowProperty (dpy, w, prop, 0, 0, FALSE,
			  AnyPropertyType, &actual_type,
			  &actual_format, &nitems, &length,
			  &data) != Success)
    return;

  if (XGetWindowProperty (dpy, w, prop, 0, length, TRUE,
			  actual_type, &actual_type,
			  &actual_format, &nitems, &length,
			  &data) != Success)
    return;

  setting.type = data[0];
  name_len = *((CARD16 *)(data + 2));
  setting.name = malloc (name_len + 1);
  memcpy (setting.name, data + 4, name_len);
  setting.name[name_len] = 0;
  p = (data + 4 + ((name_len + 3) & ~3));
  switch (setting.type)
    {
    case XSETTINGS_TYPE_STRING:
      len = *((CARD32 *)p);
      setting.data.v_string = malloc (len + 1);
      memcpy (setting.data.v_string, p + 4, len);
      setting.data.v_string[len] = 0;
      break;

    case XSETTINGS_TYPE_INT:
      setting.data.v_int = *((CARD32 *)p);
      break;

    case XSETTINGS_TYPE_COLOR:
      setting.data.v_color.red = *((CARD16 *)p);
      setting.data.v_color.green = *((CARD16 *)p + 1);
      setting.data.v_color.blue = *((CARD16 *)p + 2);
      setting.data.v_color.alpha = *((CARD16 *)p + 3);
      break;

    case 0xff:
      break;

    default:
      XFree (data);
      free (setting.name);
      return;
    }

  XFree (data);

  db_store_setting (database, &setting);
  xsettings_manager_set_setting (manager, &setting);
}

void
terminate_cb (void *data)
{
  gboolean *terminated = data;
  
  g_print ("Releasing the selection and exiting\n");

  *terminated = TRUE;
}

int 
main (int argc, char **argv)
{
  gboolean *terminated = FALSE;
  char *disp = NULL;
  Display *dpy;
  Window win;

  if ((dpy = XOpenDisplay (disp)) == NULL)
    {
      fprintf (stderr, "Cannot connect to X server on display %s.",
	      XDisplayName (disp));
      exit (1);
    }

  if (xsettings_manager_check_running (dpy, DefaultScreen (dpy)))
    {
      fprintf (stderr, "XSettings manager already running on display %s\n",
	       XDisplayName (disp));
      exit (1);
    }

  manager = xsettings_manager_new (dpy, DefaultScreen (dpy),
				   terminate_cb, &terminated);
  if (!manager)
    {
      fprintf (stderr, "Could not create manager!\n");
      exit (1);
    }

  win = xsettings_manager_get_window (manager);

  database = database_open ();

  if (!database)
    {
      fprintf (stderr, "Could not open database!\n");
      exit (1);
    }

  suck_in_settings (manager, database);

  gpe_settings_update_atom = XInternAtom (dpy, "_GPE_SETTINGS_UPDATE", 0);
  XSetSelectionOwner (dpy, gpe_settings_update_atom, win, CurrentTime);

  while (! terminated)
    {
      XEvent an_event;

      XNextEvent (dpy, &an_event);

      if (xsettings_manager_process_event (manager, (XEvent *)&an_event))
	continue;
      
      if (an_event.type == ClientMessage)
	{
	  XClientMessageEvent *cev = (XClientMessageEvent *)&an_event;
	  if (cev->message_type == gpe_settings_update_atom)
	    {
	      if (cev->format == 32)
		{
		  write_setting (cev->display, cev->window, cev->data.l[0]);
		  xsettings_manager_notify (manager);
		}
	    }
	}
    }

  xsettings_manager_destroy (manager);

  return 0;
}
