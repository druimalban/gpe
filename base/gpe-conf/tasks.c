/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *	              2003  Florian Boor <florian.boor@kernelconcepts.de>
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

#include "tasks.h"
#include "suid.h"

void task_change_nameserver(char *newns)
{
	if (!geteuid())
		change_cfg_value ("/etc/resolv.conf", "nameserver", newns, ' ');
	else
		fprintf(stderr,"Only root is allowed to change the nameserver!\n");
	usleep(200000);
}
