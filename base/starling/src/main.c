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

#include <gpe/init.h>

#include "callbacks.h"
#include "config.h"
#include "interface.h"

int main(int argc, char *argv[])
{
    Starling st;	

    setlocale (LC_ALL, "");
    bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
    textdomain (PACKAGE);
    
    gpe_application_init (&argc, &argv);

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
