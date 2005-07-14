/* GPE Mileage
 * Copyright (C) 2005  Rene Wagner <rw@handhelds.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtk/gtk.h>

#ifdef USE_GCONF
# include <gconf/gconf-client.h>

static GConfClient *gconf_client;
static gchar *gconf_path;

#else
# include "gconf-utility.h"

const static gchar *gpe_dir = ".gpe";
const static gchar *key_file_name = "mileage.settings";
static gchar *key_file_path = NULL;
static GKeyFile *key_file = NULL;

#endif /* USE_GCONF */

#include "units.h"
#include "preferences.h"

static GConfEnumStringPair unit_types[] = {
  {UNITS_LENGTH, "length"},
  {UNITS_CAPACITY, "capacity"},
  {UNITS_MONETARY, "monetary"},
  {UNITS_MILEAGE, "mileage"},
  {-1, NULL}
};

static GConfEnumStringPair length_units[] = {
  {UNITS_KILOMETER, "km"},
  {UNITS_MILE, "mile"},
  {-1, NULL}
};

static GConfEnumStringPair capacity_units[] = {
  {UNITS_LITER, "liter"},
  {UNITS_GB_GALLON, "gb_gallon"},
  {UNITS_US_GALLON, "us_gallon"},
  {-1, NULL}
};

static GConfEnumStringPair monetary_units[] = {
  {UNITS_EUR, "eur"},
  {UNITS_GBP, "gbp"},
  {UNITS_USD, "usd"},
  {-1, NULL}
};

static GConfEnumStringPair mileage_units[] = {
  {UNITS_LP100KM, "lp100km"},
  {UNITS_MPG, "mpg"},
  {-1, NULL}
};

typedef struct {
  PreferencesDomain domain;
  PreferencesNotifyFunc func;
  gpointer user_data;
} CallbackInfo;

static GSList *notify_callbacks = NULL;
static gboolean preferences_initialized = FALSE;

gint
preferences_init ()
{
  if (!preferences_initialized)
    {
#ifdef USE_GCONF
      gconf_client = gconf_client_get_default ();
      gconf_client_set_error_handling (gconf_client, GCONF_CLIENT_HANDLE_ALL);

      gconf_path = g_strdup_printf (GCONF_PATH);
      gconf_client_add_dir (gconf_client, gconf_path,
			    GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
#else
      const gchar *home = g_get_home_dir ();
      gboolean success = FALSE;

      key_file_path = g_build_filename (home, gpe_dir, key_file_name, NULL);

      /* g_print ("Using key file: %s\n", key_file_path); */

      key_file = g_key_file_new ();
      success = g_key_file_load_from_file (key_file,
					   key_file_path, G_KEY_FILE_NONE,
					   NULL);

      if (!success)
	{
	  /* g_print ("Creating new key file\n"); */
	  g_key_file_free (key_file);
	  key_file = g_key_file_new ();

	  /* units group */
	  g_key_file_set_string (key_file, "units",
				 gconf_enum_to_string (unit_types, UNITS_LENGTH),
				 gconf_enum_to_string (length_units, UNITS_DEFAULT));
	  g_key_file_set_string (key_file, "units",
				 gconf_enum_to_string (unit_types, UNITS_CAPACITY),
				 gconf_enum_to_string (capacity_units, UNITS_DEFAULT));
	  g_key_file_set_string (key_file, "units",
				 gconf_enum_to_string (unit_types, UNITS_MONETARY),
				 gconf_enum_to_string (monetary_units, UNITS_DEFAULT));
	  g_key_file_set_string (key_file, "units",
				 gconf_enum_to_string (unit_types, UNITS_MILEAGE),
				 gconf_enum_to_string (mileage_units, UNITS_DEFAULT));
	}
#endif /* USE_GCONF */
      preferences_initialized = TRUE;
    }
  return 0;
}

gint
preferences_shutdown ()
{
  if (preferences_initialized)
    {
#ifdef USE_GCONF
      gconf_client_remove_dir (gconf_client, gconf_path, NULL);
      g_free (gconf_path);

      g_object_unref (gconf_client);
#else
      /* g_print ("Saving preferences\n"); */
      if (key_file)
	{
	  GIOChannel *f = NULL;
	  GIOStatus status;
	  GError *error = NULL;
	  gint buf_size = 0, bytes_written = 0;
	  gchar *data = NULL;

	  /* g_print ("Writing to: %s\n", key_file_path); */
	  f = g_io_channel_new_file (key_file_path, "w+", &error);
	  if (error)
	    {
	      g_printerr ("Error creating IO channel: %s\n", error->message);
	      g_error_free (error);

	      return -1;
	    }
	  g_free (key_file_path);

	  data = g_key_file_to_data (key_file, &buf_size, &error);
	  if (error)
	    {
	      g_printerr ("Error writing preferences: %s\n", error->message);
	      g_error_free (error);

	      g_io_channel_unref (f);
	      g_key_file_free (key_file);
	      if (data)
		g_free (data);

	      return -1;
	    }
	  g_key_file_free (key_file);

	  status =
	    g_io_channel_write_chars (f, data, buf_size, &bytes_written,
				      &error);
	  /* FIXME: retry if status == G_IO_STATUS_AGAIN? */
	  if (error)
	    {
	      g_printerr ("Error writing preferences: %s\n", error->message);
	      g_error_free (error);

	      g_io_channel_unref (f);
	      if (data)
		g_free (data);
	      return -1;
	    }
	  g_free (data);

	  status = g_io_channel_shutdown (f, TRUE, &error);
	  while (!error && status == G_IO_STATUS_AGAIN)
	    status = g_io_channel_shutdown (f, TRUE, &error);
	  if (error)
	    {
	      g_printerr ("Error writing preferences: %s\n", error->message);
	      g_error_free (error);

	      g_io_channel_unref (f);
	      return -1;
	    }

	  g_io_channel_unref (f);
	}
#endif /* USE_GCONF */
    }
  return 0;
}

gint
preferences_get_unit (UnitType unit_type)
{
  if (preferences_initialized)
    {
#ifdef USE_GCONF
      gchar *result;

      gchar *key = g_strdup_printf ("%s/homepage", gconf_path);

      result = gconf_client_get_string (gconf_client, key, NULL);
      g_free (key);

      if (result && (g_utf8_strlen (result, -1) > 0))
	return result;
#else
      const gchar *unit = gconf_enum_to_string (unit_types, unit_type);
      gchar *value = NULL;

      value = g_key_file_get_string (key_file, "units", unit, NULL);
      if (value)
	{
	  gint int_val = 0;
	  gboolean success = FALSE;
	  switch (unit_type)
	    {
	      case UNITS_LENGTH:
                success = gconf_string_to_enum (length_units, value, &int_val);
		break;
	      case UNITS_CAPACITY:
                success = gconf_string_to_enum (capacity_units, value, &int_val);
		break;
	      case UNITS_MONETARY:
                success = gconf_string_to_enum (monetary_units, value, &int_val);
		break;
	      case UNITS_MILEAGE:
	        success = gconf_string_to_enum (mileage_units, value, &int_val);
		break;
	    }
	  if (success)
	    {
	      g_free (value);
	      return int_val;
	    }

	  g_free (value);
	}
#endif /* USE_GCONF */
    }
  return UNITS_DEFAULT;
}

void
notify_all_in_domain (PreferencesDomain domain)
{
  GSList *iter;

  for (iter = notify_callbacks; iter; iter = iter->next)
    {
      CallbackInfo *c = (CallbackInfo *)(iter->data);

      if (domain == c->domain)
        ((PreferencesNotifyFunc) (*c->func)) (c->domain, c->user_data);
    }
}

gboolean
preferences_set_unit (UnitType unit_type, gint value)
{
  if (!preferences_initialized)
    return FALSE;

#ifdef USE_GCONF
#else
  const gchar *str_val = NULL;

  switch (unit_type)
    {
      case UNITS_LENGTH:
        str_val = gconf_enum_to_string (length_units, value);
	break;
      case UNITS_CAPACITY:
        str_val = gconf_enum_to_string (capacity_units, value);
	break;
      case UNITS_MONETARY:
        str_val = gconf_enum_to_string (monetary_units, value);
	break;
      case UNITS_MILEAGE:
        str_val = gconf_enum_to_string (mileage_units, value);
	break;
    }

  if (!str_val)
    return FALSE;

  g_key_file_set_string (key_file, "units", gconf_enum_to_string (unit_types, unit_type), str_val);
  
  notify_all_in_domain (PREFERENCES_DOMAIN_UNITS);
#endif /* USE_GCONF */

  return TRUE;
}

void
preferences_notify_add (PreferencesDomain domain, PreferencesNotifyFunc func, gpointer user_data)
{
  CallbackInfo *c = g_new0 (CallbackInfo, 1);

  c->domain = domain;
  c->func = func;
  c->user_data = user_data;
  
  notify_callbacks = g_slist_append (notify_callbacks, c);
}
