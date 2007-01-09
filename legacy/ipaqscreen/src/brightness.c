#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "brightness.h"

void set_brightness (int brightness)
{
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

        pipe = popen ("bl", "r");

        if (pipe > 0)
        {
                if (fscanf (pipe, "%4s %d", state, &brightness) <= 0)
                {
                        fprintf (stderr, "can't interpret output from bl\n");
			strcpy (state, "on");
			brightness = 255;
                }
                pclose (pipe);
        }
        else
        {
                fprintf (stderr, "couldn't read brightness");
                brightness = 255;
        }

	if (strcmp (state, "off") == 0)
	{
		brightness = 0;
	}
	return brightness;
}
