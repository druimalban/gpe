/*
 * Copyright (C) 2005 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <libgen.h>
#include <stdio.h>

const char *rewrite_root;
int rewrite_root_len;

void rewrite_init (void) __attribute__((constructor));

int (*real_open) (const char *name, int flags, mode_t mode);
int (*real_opendir) (const char *name);
int (*real_stat) (int ver, const char *name, struct stat *s);
int (*real_stat64) (int ver, const char *name, struct stat64 *s);
int (*real_lstat) (int ver, const char *name, struct stat *s);
int (*real_lstat64) (int ver, const char *name, struct stat64 *s);

char *
mangle_path (const char *old_path, int is_write)
{
  char *new_path;

  if (rewrite_root == NULL)
    return NULL;

  if (access (old_path, F_OK) == 0)
    return NULL;

  if (old_path[0] != '/')
    return NULL;

  new_path = malloc (strlen (old_path) + rewrite_root_len + 1);
  sprintf (new_path, "%s%s", rewrite_root, old_path);

  if (access (new_path, F_OK) == 0)
    return new_path;

  if (is_write)
    {
      char *tmp_path, *dir;

      tmp_path = strdup (new_path);
      dir = dirname (tmp_path);
      if (!strcmp (dir, "."))
	{
	  free (tmp_path);
	  free (new_path);
	  return NULL;
	}

      if (access (tmp_path, F_OK) == 0)
	{
	  free (tmp_path);
	  return new_path;
	}

      free (tmp_path);
    }

  free (new_path);
  return NULL;
}

int
rewrite_open (const char *name, int flags, mode_t mode)
{
  int r;
  char *p;
  int is_write;

  is_write = ((flags & O_WRONLY) == O_WRONLY) || ((flags & O_RDWR) == O_RDWR);

  p = mangle_path (name, is_write);

  r = real_open (p ? p : name, flags, mode);
  
  if (p)
    free (p);

  return r;
}
asm (".set open,rewrite_open; .globl open");

int
rewrite_opendir (const char *name)
{
  int r;
  char *p;

  p = mangle_path (name, 0);

  r = real_opendir (p ? p : name);
  
  if (p)
    free (p);

  return r;
}
asm (".set opendir,rewrite_opendir; .globl opendir");

int
rewrite_stat (int ver, const char *name, struct stat *s)
{
  int r;
  char *p;

  p = mangle_path (name, 0);

  r = real_stat (ver, p ? p : name, s);
  
  if (p)
    free (p);

  return r;
}
asm (".set __xstat,rewrite_stat; .globl __xstat");

int
rewrite_lstat (int ver, const char *name, struct stat *s)
{
  int r;
  char *p;

  p = mangle_path (name, 0);
  
  r = real_lstat (ver, p ? p : name, s);
  
  if (p)
    free (p);

  return r;
}
asm (".set __lxstat,rewrite_lstat; .globl __lxstat");

int
rewrite_stat64 (int ver, const char *name, struct stat64 *s)
{
  int r;
  char *p;

  p = mangle_path (name, 0);

  r = real_stat64 (ver, p ? p : name, s);
  
  if (p)
    free (p);

  return r;
}
asm (".set __xstat64,rewrite_stat64; .globl __xstat64");

int
rewrite_lstat64 (int ver, const char *name, struct stat64 *s)
{
  int r;
  char *p;

  p = mangle_path (name, 0);

  r = real_lstat64 (ver, p ? p : name, s);
  
  if (p)
    free (p);

  return r;
}
asm (".set __lxstat64,rewrite_lstat64; .globl __lxstat64");

void
rewrite_init (void)
{
  rewrite_root = getenv ("REWRITE_ROOT");

  if (rewrite_root)
    rewrite_root_len = strlen (rewrite_root);

  real_open = dlsym (RTLD_NEXT, "open");
  real_opendir = dlsym (RTLD_NEXT, "opendir");
  real_stat = dlsym (RTLD_NEXT, "__xstat");
  real_lstat = dlsym (RTLD_NEXT, "__lxstat");
  real_stat64 = dlsym (RTLD_NEXT, "__xstat64");
  real_lstat64 = dlsym (RTLD_NEXT, "__lxstat64");
}
