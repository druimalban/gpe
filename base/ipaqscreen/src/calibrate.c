#include <unistd.h>

#include "calibrate.h"

void calibrate ()
{
	int pid;
	pid = fork();
	if (pid == 0)
	{
                 execlp ("xcalibrate", "xcalibrate", 0);
                 exit (0);
	}
}
