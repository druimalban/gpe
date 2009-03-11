/* gpe-exec.c - Execute some command.

   Original code
   Copyright (C) 1998 Tom Tromey
   modified for GPE
   2002 by Nils Faerber <nils@kernelconcepts.de>

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef _GPE_EXEC_H
#define _GPE_EXEC_H

#include <glib.h>

/* Fork and execute some program in the background.  Returns -1 on
   error.  Returns PID on success.  Should correctly report errno
   returns from a failing child invocation.  DIR is the directory in
   which to exec the child; if NULL the current directory is used.
   Searches $PATH to find the child.  */
int gpe_execute_async (const char *dir, int argc, char * const argv[]);
int gpe_execute_async_fds (const char *dir, int argc, char * const argv[], 
			     gboolean close_fds);

/* Like gpe_execute_async, but each string in ENVV is added to the
   child's environment.  If you want to set the environment exactly,
   you must set the global `environ' variable instead.  If ENVV is
   NULL, the child inherits the parent's environment.  In this case,
   the value of ENVC is ignored.  */
int gpe_execute_async_with_env (const char *dir,
				  int argc, char * const argv[],
				  int envc, char * const envv[]);
int gpe_execute_async_with_env_fds (const char *dir, int argc, 
				      char * const argv[], int envc, 
				      char * const envv[], gboolean close_fds);

int gpe_execute_async_cmd(char *cmd);

#endif
