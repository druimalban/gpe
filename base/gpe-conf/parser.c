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

#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <gtk/gtk.h>

       int vsscanf (const char *str, const char *format, va_list ap);

/* some very modest parser functions.. */

static int vparse(FILE *f, char *format, va_list ap)
{
  char buffer[256];
  fgets (buffer, 255, f);
  while ((feof(f) == 0))
    {
      if ( vsscanf(buffer, format, ap) > 0)
	{
	  return 0;
	}
      fgets (buffer, 255, f);
    }
  return 1;
}
int parse_pipe(char *cmd,char *format,...)
{
  va_list ap;
  FILE *pipe;
  int rv = 1;
  va_start(ap, format);

  pipe = popen (cmd, "r");

  if (pipe > 0)
    {
      rv = vparse(pipe,format,ap);
      pclose (pipe);
    }
  va_end(ap);
  return rv;
}
int parse_file(char *file,char *format,...)
{
  va_list ap;
  FILE *f;
  int rv = 1;
  va_start(ap, format);

  f = fopen (file, "r");

  if (pipe > 0)
    {
      rv = vparse(f,format,ap);
      fclose (f);
    }
  va_end(ap);
  return rv;
}

int parse_file_and_gfree(char *file,char *format,...)
{
  va_list ap;
  FILE *f;
  int rv = 1;
  va_start(ap, format);

  f = fopen (file, "r");

  if (pipe > 0)
    {
      rv = vparse(f,format,ap);
      fclose (f);
    }
  va_end(ap);
  g_free(file);
  return rv;
}
int parse_pipe_and_gfree(char *cmd,char *format,...)
{
  va_list ap;
  FILE *pipe;
  int rv = 1;
  va_start(ap, format);

  pipe = popen (cmd, "r");

  if (pipe > 0)
    {
      rv = vparse(pipe,format,ap);
      pclose (pipe);
    }
  g_free(cmd);
  va_end(ap);
  return rv;

}
