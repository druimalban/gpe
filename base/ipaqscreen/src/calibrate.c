#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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
	else if (pid > 0)
	{
	        waitpid (pid, NULL, 0);
	}
}
