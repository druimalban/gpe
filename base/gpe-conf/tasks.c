/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *	             2003, 2004  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Module containing command line config tasks.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
 
#include "tasks.h"
#include "suid.h"
#include "sound/soundctrl.h"

/* change primary nameserver in resolv.conf */
void 
task_change_nameserver(char *newns)
{
	if (!geteuid())
		change_cfg_value ("/etc/resolv.conf", "nameserver", newns, ' ');
	else
		fprintf(stderr,
		       "Only root is allowed to change the nameserver!\n");
	usleep(200000);
}

/* load/save sound settings */
void 
task_sound(char *action)
{
	if (!action) 
		return;
	if ((action[0] == 's') || !strcmp(action, "save"))
	{
		sound_init();
		sound_save_settings();
	}
	else if ((action[0] == 'r') || !strcmp(action, "restore"))
	{
		sound_init();
		
		if (get_mute_status())
			set_mute_status(TRUE);
	}
	else
		fprintf(stderr, "Invalid argument given\n");
}
