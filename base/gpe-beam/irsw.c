/*
 * This is gpe-beam, a simple IrDa frontend for GPE
 *
 * suid ir switching tool
 *
 * Copyright (c) 2003, Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define COMMAND_IR_ON  "/sbin/ifconfig irda0 up ; echo 1 > /proc/sys/net/irda/discovery"
#define COMMAND_IR_OFF  "echo 0 > /proc/sys/net/irda/discovery ; /sbin/ifconfig irda0 down"
#define COMMAND_ATTACH PREFIX "/sbin/irattach pxa_ir"

void usage()
{
	fprintf(stderr,"Usage: irsw [on/off]\n\n");
}

int
main(int argc, char *argv[])
{
	if (argc !=2)
	{
		usage();
		exit(-1);
	}
	setuid(0);
	seteuid(0);
	if (!strcmp(argv[1],"on"))
	{
		system(COMMAND_ATTACH);
		system(COMMAND_IR_ON);
		exit(0);
	}
	if (!strcmp(argv[1],"off"))
	{
		system(COMMAND_IR_OFF);
		exit(0);
	}
	usage();
	exit(-1);
}
