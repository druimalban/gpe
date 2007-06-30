/*
* This file is part of GPE-Memo
*
* Copyright (C) 2007 Alberto García Hierro
*	<skyhusker@rm-fr.net>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version
* 2 of the License, or (at your option) any later version.
*/

#include <gtk/gtkmain.h>

#include <gpe/init.h>

#include <string.h>

#include "interface.h"

int main (int argc, char **argv)
{
    Interface in;

    gpe_application_init (&argc, &argv);

    if (argc > 1 && strcmp (argv[1], "--record") == 0) {
        interface_record (NULL, argv[2]);
    } else {
        interface_init (&in);
        gtk_main();
    }

    return 0;
}
