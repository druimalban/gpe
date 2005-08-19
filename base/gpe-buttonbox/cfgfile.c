/*
 * Copyright (C) 2005 Florian Boor <florian@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include "cfgfile.h"
#include "globals.h"

extern t_cfg cfg;

gboolean
config_load(void)
{
  GKeyFile *configfile;
  GError *err = NULL;
  gchar *fname = g_strdup_printf("%s/%s", g_get_home_dir(), CONFIGFILE);
  gint value;

  configfile = g_key_file_new();

  cfg.slot_width = SLOT_WIDTH_DEFAULT;
  cfg.slot_height = SLOT_HEIGHT_DEFAULT;
  cfg.nr_slots = NR_SLOTS_DEFAULT;
  cfg.icon_size = ICON_SIZE_DEFAULT;
  cfg.myfiles_on = MYFILES_DEFAULT;
  cfg.labels_on = LABLES_DEFAULT;

  cfg.fixed_slots = 1;
	
  if (!g_key_file_load_from_file (configfile, fname,
                                  G_KEY_FILE_KEEP_COMMENTS, &err)
      && !g_key_file_load_from_file (configfile, "/etc/gpe/" CONFIGFILE,
                                  G_KEY_FILE_KEEP_COMMENTS, &err))
    {
      g_error_free(err);
      g_free(fname);
      return FALSE;
    }
    
  if (g_key_file_has_group(configfile, "Global"))
    {
      value = g_key_file_get_integer(configfile, 
                                     "Global", "slot_width", &err);
      if (err)
        {
            g_error_free(err);
            err = NULL;
        }
      else
		cfg.slot_width = value;
      value = g_key_file_get_integer(configfile, 
                                     "Global", "slot_height", &err);
      if (err)
        {
            g_error_free(err);
            err = NULL;
        }
      else
		cfg.slot_height = value;
      value = g_key_file_get_integer(configfile, 
                                     "Global", "slots", &err);
      if (err)
        {
          g_error_free(err);
          err = NULL;
        }
      else
		cfg.nr_slots = value;
      value = g_key_file_get_integer(configfile, 
                                     "Global", "icon_size", &err);
      if (err)
        {
          g_error_free(err);
          err = NULL;
        }
      else
		cfg.icon_size = value;
      value = g_key_file_get_boolean(configfile, 
                                     "Global", "show_myfiles", &err);
      if (err)
        {
          g_error_free(err);
          err = NULL;
        }
	  else
		cfg.myfiles_on = value;
      value = g_key_file_get_boolean(configfile, 
                                     "Global", "show_labels", &err);
      if (err)
        {
          g_error_free(err);
          err = NULL;
        }
	  else
		cfg.labels_on = value;
    }
  if (cfg.myfiles_on)
      cfg.fixed_slots = 2;
  g_key_file_free(configfile);
  g_free(fname);
  return TRUE;
}

gboolean
config_save(void)
{
  GKeyFile *configfile;
  GError *err = NULL;
  gchar *fname = g_strdup_printf("%s/%s", g_get_home_dir(), CONFIGFILE);
  gsize len;
  FILE *fp;

  configfile = g_key_file_new();
  g_key_file_set_integer(configfile, "Global", "slot_width", cfg.slot_width);
  g_key_file_set_integer(configfile, "Global", "slot_height", cfg.slot_height);
  g_key_file_set_integer(configfile, "Global", "slots", cfg.nr_slots);
  g_key_file_set_integer(configfile, "Global", "icon_size", cfg.icon_size);
  g_key_file_set_boolean(configfile, "Global", "show_myfiles", cfg.myfiles_on);
  g_key_file_set_boolean(configfile, "Global", "show_labels", cfg.labels_on);

  fp = fopen(fname, "w");
  if (fp)
     fprintf(fp, "%s", g_key_file_to_data(configfile, &len, &err));
  else
    return FALSE;
  
  fclose(fp);
  if (err)
    {
      g_error_free(err);
      return FALSE;
    }
  g_free(fname);
  return TRUE;
}
