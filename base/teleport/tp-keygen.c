/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <gcrypt.h>
#include <unistd.h>
#include <fcntl.h>

#include "auth.h"

static gchar *home_dir;

static GcryMPI
mpi_from_sexp (GcrySexp r, char *tag)
{
  GcrySexp s = gcry_sexp_find_token (r, tag, 0);
  return gcry_sexp_nth_mpi (s, 1, GCRYMPI_FMT_STD);
}

char *
hex_from_mpi (GcryMPI m)
{
  char *buf;

  gcry_mpi_aprint (GCRYMPI_FMT_HEX, (void *)&buf, NULL, m);

  return buf;
}

int
generate_key (struct rsa_key *rv)
{
  int rc;
  GcrySexp r, parms;
 
  rc = gcry_sexp_build (&parms, NULL, "(genkey (rsa (nbits %d)))", 1024);
  if (rc)
    return rc;

  rc = gcry_pk_genkey (&r, parms);
  gcry_sexp_release (parms);
  if (rc)
    return rc;

  rv->n = mpi_from_sexp (r, "n");
  rv->d = mpi_from_sexp (r, "e");
  rv->e = mpi_from_sexp (r, "d");
  rv->p = mpi_from_sexp (r, "p");
  rv->q = mpi_from_sexp (r, "q");
  rv->u = mpi_from_sexp (r, "u");

  gcry_sexp_release (r);

  return 0;
}

gboolean
write_public (struct rsa_key *k)
{
  gchar *filename = g_strdup_printf ("%s/.gpe/migrate/public", home_dir);
  int fd;
  char *n, *e, *id;
  size_t len;
  gchar *buf;
  
  fd = open (filename, O_WRONLY | O_CREAT | O_APPEND, 0600);
  g_free (filename);
  if (fd < 0)
    return FALSE;

  n = hex_from_mpi (k->n);
  e = hex_from_mpi (k->e);
  len = strlen (n);
  id = n + (len - 8);

  buf = g_strdup_printf ("%s %s %s\n", id, e, n);

  gcry_free (n);
  gcry_free (e);

  write (fd, buf, strlen (buf));

  g_free (buf);

  close (fd);
  return TRUE;
}

gboolean
write_secret (struct rsa_key *k)
{
  gchar *filename = g_strdup_printf ("%s/.gpe/migrate/secret", home_dir);
  int fd;
  char *n, *d, *id, *p, *q, *u, *e;
  size_t len;
  gchar *buf;
  
  fd = open (filename, O_WRONLY | O_CREAT | O_APPEND, 0600);
  g_free (filename);
  if (fd < 0)
    return FALSE;

  n = hex_from_mpi (k->n);
  e = hex_from_mpi (k->e);
  d = hex_from_mpi (k->d);
  p = hex_from_mpi (k->p);
  q = hex_from_mpi (k->q);
  u = hex_from_mpi (k->u);
  len = strlen (n);
  id = n + (len - 8);

  buf = g_strdup_printf ("%s %s %s %s %s %s %s\n", id, e, d, n, p, q, u);

  gcry_free (n);
  gcry_free (d);
  gcry_free (p);
  gcry_free (q);
  gcry_free (u);

  write (fd, buf, strlen (buf));

  g_free (buf);

  close (fd);
  return TRUE;
}

int
main ()
{
  struct rsa_key k;
  gchar *d;

  home_dir = g_get_home_dir ();

  d = g_strdup_printf ("%s/.gpe", home_dir);
  mkdir (d, 0700);
  g_free (d);
  d = g_strdup_printf ("%s/.gpe/migrate", home_dir);
  mkdir (d, 0700);
  g_free (d);

  gcry_control (GCRYCTL_INIT_SECMEM, 1);
  gcry_check_version (NULL);

  generate_key (&k);

  write_secret (&k);
  write_public (&k);

  exit (0);
}
