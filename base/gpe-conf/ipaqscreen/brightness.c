/*
 * gpe-conf
 *
 * Copyright (C) 2002 Moray Allan <moray@sermisy.org>, Pierre TARDY <tardyp@free.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

#include "gpe/errorbox.h"
#include <sys/types.h>
#include <sys/wait.h>

#include "brightness.h"

void set_brightness (int brightness)
{
#ifdef __i386__
	return ; // bl doesnt exit on i386 dev machines!
#endif
	if (brightness == 0)
	{
		system ("bl off\n");
	}
	else
	{
		int pid;
		pid = fork ();
		if (pid == 0)
		{
			char s[10];
			sprintf (s, "%d", brightness);
			execlp ("bl", "bl", s, 0);
			exit(0);
		}
		else if (pid > 0)
		{
			waitpid (pid, NULL, 0);
		}
	}
}

int get_brightness ()
{
  
        FILE *pipe;
        int brightness;
	char state[5];
#ifdef __i386__
	return 10; // bl doesnt exit on i386 dev machines!
#endif
        pipe = popen ("bl", "r");

        if (pipe > 0)
        {
                if (fscanf (pipe, "%4s %d", state, &brightness) <= 0)
                {
                        gpe_error_box ( "can't interpret output from bl\n");
			strcpy (state, "on");
			brightness = 255;
                }
                pclose (pipe);
        }
        else
        {
                gpe_error_box ( "couldn't read brightness");
                brightness = 255;
        }

	if (strcmp (state, "off") == 0)
	{
		brightness = 0;
	}
	return brightness;
}
