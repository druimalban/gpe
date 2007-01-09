#include "support.h"

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "rotation.h"

extern int initialising;

char * get_rotation ()
{
        FILE *pipe;
        int rotation;

        pipe = popen ("xrandr", "r");

        if (pipe > 0)
        {
		rotation = -1;
		while ((feof(pipe) == 0) && (rotation < 0))
		{
		  char buffer[256], buffer2[20];
		  fgets (buffer, 255, pipe);
		  if (sscanf (buffer, "Current rotation - %20s", buffer2) > 0)
		  {
		    if (strcmp (buffer2, "normal") == 0)
		    {
		      rotation = 0;
		    }
		    else if (strcmp (buffer2, "left") == 0)
		    {
		      rotation = 1;
		    }
		    else if (strcmp (buffer2, "inverted") == 0)
		    {
		      rotation = 2;
		    }
		    else if (strcmp (buffer2, "right") == 0)
		    {
		      rotation = 3;
		    }
                  }
		}
		if (rotation < 0)
		{
			fprintf (stderr, "can't interpret output from xrandr\n");
			rotation = 0;
		}
                pclose (pipe);
        }
        else
        {
                fprintf (stderr, "couldn't read rotation\n");
		rotation = 0;
        }
	switch (rotation)
	{
	case 0:
		return (_("Portrait"));
	case 1:
		return (_("Landscape (left)"));
	case 2:
		return (_("Inverted"));
	case 3:
		return (_("Landscape (right)"));
	}
	return 0;
}

void set_rotation (char *rotation)
{
	if (initialising == 0)
	{
		int pid;
		pid = fork();
		if (pid == 0)
		{
	                 execlp ("xrandr", "xrandr", "-o", rotation, 0);
	                 exit (0);
		}
		else if (pid > 0)
		{
		        waitpid (pid, NULL, 0);
		}
	}
}
