/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
//#define _XOPEN_SOURCE /* Pour GlibC2 */
#include <time.h>
#include <errno.h>

char *allowed_file[] =
  {
    "/etc/xcalibrate.conf",
    "/etc/keyctl.conf"

  };
int filenb= sizeof(allowed_file)/sizeof(allowed_file[0]);

char *allowed_cmd[] = 
{
  "/sbin/ifconfig"
};

int main(int argc, char **argv)
{
  int i;
  if( argc > 1)
    {
      if(strcmp(argv[1],"--stime")==0)
	{
	  time_t t = atoi(argv[2]);
	  if(stime(&t) == -1)
	    {
	      exit( errno);
	    }
	}
      if(strcmp(argv[1],"--sntime")==0)
	{
	  execlp("/usr/sbin/ntpdate","/usr/sbin/ntpdate",argv[2],0);
	}
#if 0
      if(strcmp(argv[1],"--writefile")==0)
	{
	  for(i=0 ; i<filenb;i++)
	    {
	      if(strcmp(argv[2],allowed_file))
		
	    }
	  
	}
#endif
    }

}
