/*
 * Copyright (C) 2006 Alberto García Hierro
 *      <skyhusker@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gst/gst.h>

#include <gtk/gtkmain.h>

#ifdef ENABLE_GPE
#   include <gpe/init.h>
#endif

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

    play_list_init (&argc, &argv);
    
    interface_init (&st);
    
    player_init (&st);

    callbacks_setup (&st);

    lyrics_init ();

    lastfm_init ();

    config_init (&st);

    config_load (&st);
    
    gtk_main();
    
    return 0;
}
