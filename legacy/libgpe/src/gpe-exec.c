/* gnome-exec.c - Execute some command.

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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

#include <gpe-exec.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#include <assert.h>

static void
set_cloexec (int fd)
{
  fcntl (fd, F_SETFD, FD_CLOEXEC);
}

/**
 * gpe_execute_async_with_env_fds:
 * @dir: Directory in which child should be execd, or NULL for current
 *       directory
 * @argc: Number of arguments
 * @argv: Argument vector to exec child
 * @envc: Number of environment slots
 * @envv: Environment vector
 * @close_fds: If TRUE will close all fds but 0,1, and 2
 * 
 * Like gpe_execute_async_with_env() but has a flag to decide whether or not  * to close fd's
 * 
 * Return value: the process id, or %-1 on error.
 **/
int
gpe_execute_async_with_env_fds (const char *dir, int argc, 
				       char * const argv[], int envc, 
				       char * const envv[], 
				       gboolean close_fds)
{
  int parent_comm_pipes[2], child_comm_pipes[2];
  int child_errno, itmp, i, open_max;
  int res;
  char **cpargv;
  pid_t child_pid, immediate_child_pid; /* XXX this routine assumes
					   pid_t is signed */

  if(pipe(parent_comm_pipes))
    return -1;

  child_pid = immediate_child_pid = fork();

  switch(child_pid) {
  case -1:
    close(parent_comm_pipes[0]);
    close(parent_comm_pipes[1]);
    return -1;

  case 0: /* START PROCESS 1: child */
    child_pid = -1;
    res = pipe(child_comm_pipes);
    close(parent_comm_pipes[0]);
    if(!res)
      child_pid = fork();

    switch(child_pid) {
    case -1:
      itmp = errno;
      child_pid = -1; /* simplify parent code */
      write(parent_comm_pipes[1], &child_pid, sizeof(child_pid));
      write(parent_comm_pipes[1], &itmp, sizeof(itmp));
      close(child_comm_pipes[0]);
      close(child_comm_pipes[1]);
      _exit(0); break;      /* END PROCESS 1: monkey in the middle dies */

    default:
      {
	char buf[16];
	
	close(child_comm_pipes[1]);
	while((res = read(child_comm_pipes[0], buf, sizeof(buf))) > 0)
	  write(parent_comm_pipes[1], buf, res);
	close(child_comm_pipes[0]);
	_exit(0); /* END PROCESS 1: monkey in the middle dies */
      }
      break;

    case 0:                 /* START PROCESS 2: child of child */
      close(parent_comm_pipes[1]);
      /* pre-exec setup */
      close (child_comm_pipes[0]);
      set_cloexec (child_comm_pipes[1]);
      child_pid = getpid();
      res = write(child_comm_pipes[1], &child_pid, sizeof(child_pid));

      if(envv) {
	for(itmp = 0; itmp < envc; itmp++)
	  putenv(envv[itmp]);
      }

      if(dir) chdir(dir);

      cpargv = alloca((argc + 1) * sizeof(char *));
      memcpy(cpargv, argv, argc * sizeof(char *));
      cpargv[argc] = NULL;

      if(close_fds)
	{
	  int stdinfd;
	  /* Close all file descriptors but stdin stdout and stderr */
	  open_max = sysconf (_SC_OPEN_MAX);
	  for (i = 3; i < open_max; i++)
	    set_cloexec (i);

	  if(child_comm_pipes[1] != 0) {
	    close(0);
	    /* Open stdin as being nothingness, so that if someone tries to
	       read from this they don't hang up the whole GNOME session. BUGFIX #1548 */
	    stdinfd = open("/dev/null", O_RDONLY);
	    assert(stdinfd >= 0);
	    if(stdinfd != 0)
	      {
		dup2(stdinfd, 0);
		close(stdinfd);
	      }
	  }
	}
      setsid ();
      signal (SIGPIPE, SIG_DFL);
      /* doit */
      execvp(cpargv[0], cpargv);

      /* failed */
      itmp = errno;
      write(child_comm_pipes[1], &itmp, sizeof(itmp));
      _exit(1); break;      /* END PROCESS 2 */
    }
    break;

  default: /* parent process */
    /* do nothing */
    break;
  }

  close(parent_comm_pipes[1]);

  res = read (parent_comm_pipes[0], &child_pid, sizeof(child_pid));
  if (res != sizeof(child_pid))
    {
      g_message("res is %d instead of %d", res, sizeof(child_pid));
      child_pid = -1; /* really weird things happened */
    }
  else if (read (parent_comm_pipes[0], &child_errno, sizeof(child_errno))
	  == sizeof(child_errno))
    {
      errno = child_errno;
      child_pid = -1;
    }

  /* do this after the read's in case some OS's handle blocking on pipe writes
     differently */
  waitpid(immediate_child_pid, &itmp, 0); /* eat zombies */

  close(parent_comm_pipes[0]);

  if(child_pid < 0)
    g_message("gpe_execute_async_with_env_fds: returning %d", child_pid);

  return child_pid;
}

/**
 * gpe_execute_async_with_env:
 * @dir: Directory in which child should be execd, or NULL for current
 *       directory
 * @argc: Number of arguments
 * @argv: Argument vector to exec child
 * @envc: Number of environment slots
 * @envv: Environment vector
 * 
 * This function forks and executes some program in the background.
 * On error, returns %-1; in this case, errno should hold a useful
 * value.  Searches the path to find the child.  Environment settings
 * in @envv are added to the existing environment -- they do not
 * completely replace it.  This function closes all fds besides 0, 1,
 * and 2 for the child
 * 
 * Return value: the process id, or %-1 on error.
 **/
int
gpe_execute_async_with_env (const char *dir, int argc, char * const argv[], 
			      int envc, char * const envv[])
{
  return gpe_execute_async_with_env_fds(dir,argc,argv,envc,envv,1);
}


/**
 * gpe_execute_async:
 * @dir: Directory in which child should be execd, or NULL for current
 *       directory
 * @argc: Number of arguments
 * @argv: Argument vector to exec child
 * 
 * Like gpe_execute_async_with_env(), but doesn't add anything to
 * child's environment.
 * 
 * Return value: process id of child, or %-1 on error.
 **/
int
gpe_execute_async (const char *dir, int argc, char * const argv[])
{
  return gpe_execute_async_with_env (dir, argc, argv, 0, NULL);
}

/**
 * gpe_execute_async_fds:
 * @dir: Directory in which child should be execd, or NULL for current
 *       directory
 * @argc: Number of arguments
 * @argv: Argument vector to exec child
 * @close_fds: 
 * Like gpe_execute_async_with_env_fds(), but doesn't add anything to
 * child's environment.
 * 
 * Return value: process id of child, or %-1 on error.
 **/
int
gpe_execute_async_fds (const char *dir, int argc, 
				 char * const argv[], gboolean close_fds)
{
  return gpe_execute_async_with_env_fds (dir, argc, argv, 0, NULL, 
					   close_fds);
}

/*
 * count the arguments of a one-line command
 */
static int __gpe_count_args(char *cmd)
{
char *tcmd;
int cnt=0;

	tcmd = (char *) malloc ((strlen(cmd) + 1) * sizeof(char));
	strcpy(tcmd, cmd);
	if (strtok(tcmd," ") != NULL)
		cnt++;
	while (strtok(NULL," ") != NULL)
		cnt++;
	free(tcmd);

return cnt;
}

/*
 * Here we execute a single-line command ripping the commandline apart
 * into single argv[]s so that we can feed it into gpe_execute_async()
 */
int gpe_execute_async_cmd(char *cmd)
{
char *tcmd;
char **margv;
char *tmp;
int margc,i,result;

	/* that would be evil */
	if (cmd == NULL)
		return -1;
	/* and this does not make sense */
	if (strlen(cmd) == 0)
		return -1;
	margc = __gpe_count_args(cmd);
	margv = (char **)malloc(margc * sizeof(char *));
	tcmd = (char *)malloc((strlen(cmd)+1)*sizeof(char));
	strcpy(tcmd,cmd);
	tmp = strtok(tcmd," ");
	for (i=0; i<margc; i++) {
		margv[i]=(char *)malloc((strlen(tmp)+1)*sizeof(char));
		strcpy(margv[i],tmp);
		tmp=strtok(NULL," ");
	}
	free(tcmd);
	result=gpe_execute_async(NULL,margc,margv);
	for (i=0; i<margc; i++)
		free(margv[i]);
	free(margv);

return result;
}
