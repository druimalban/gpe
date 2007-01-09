/*
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/signal.h>
#include <stdlib.h>
#include <sys/wait.h>

int
main (int argc, char *argv[])
{
  setpriority (PRIO_PROCESS, 0, -19);

  if (seteuid (getuid ()))
    {
      perror ("seteuid");
      exit (EXIT_FAILURE);
    }

  if (execvp ("esd", argv))
    perror ("esd");

  exit (EXIT_FAILURE);
}
