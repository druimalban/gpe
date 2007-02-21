/*
 * Copyright (C) 2006 Alberto García Hierro
 *      <skyhusker@handhelds.org>
 * Copyright (C) 2007 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <locale.h>
#include <gst/gst.h>

#include <gtk/gtkmain.h>

#ifdef ENABLE_GPE
#   include <gpe/init.h>
#endif

#include "starling.h"
#include "callbacks.h"
#include "config.h"
#include "interface.h"

int main(int argc, char *argv[])
{
  Starling st;	

  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
    
#ifdef ENABLE_GPE
  gpe_application_init (&argc, &argv);
#else
  gtk_init (&argc, &argv);
#endif

  const char *home = g_get_home_dir ();
  char *file = g_strdup_printf ("%s/.gpe/playlist", home);

  GError *err = NULL;
  st.pl = play_list_open (file, &err);
  g_free (file);
  if (err)
    {
      g_warning (err->message);
      starling_error_box (err->message);
      g_error_free (err);
      exit (1);
    }


  interface_init (&st);
    
    callbacks_setup (&st);

    lyrics_init ();

    lastfm_init ();

    config_init (&st);

    config_load (&st);
    
    gtk_main();
    
    return 0;
}
