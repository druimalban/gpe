/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

#include <gcrypt.h>

#include "auth.h"

static struct rsa_key private_key;

static GcryMPI
mpi_from_sexp (GcrySexp r, char *tag)
{
  GcrySexp s = gcry_sexp_find_token (r, tag, 0);
  return gcry_sexp_nth_mpi (s, 1, GCRYMPI_FMT_STD);
}

static char *
hex_from_mpi (GcryMPI m)
{
  char *buf;

  gcry_mpi_aprint (GCRYMPI_FMT_HEX, (void *)&buf, NULL, m);

  return buf;
}

static gboolean
sign_hash (struct rsa_key *k, char *hash, gchar **result)
{
  GcryMPI mpi;
  GcrySexp data, sig, key;
  size_t nb = 19;
  int rc;
  char *hex;

  gcry_mpi_scan (&mpi, GCRYMPI_FMT_USG, hash, &nb);

  gcry_sexp_build (&data, NULL, "(data (flags pkcs1) (hash sha1 %m))", mpi);
  
  gcry_sexp_build (&key, NULL, "(private-key (rsa (n %m) (e %m) (d %m) (p %m) (q %m) (u %m)))",
		   k->n, k->e, k->d, k->p, k->q, k->u);

  rc = gcry_pk_sign (&sig, data, key);

  gcry_sexp_release (data);
  gcry_sexp_release (key);
  gcry_mpi_release (mpi);

  if (rc)
    return FALSE;

  mpi = mpi_from_sexp (sig, "s");
  hex = hex_from_mpi (mpi);
  *result = g_strdup (hex);
  gcry_free (hex);
  gcry_mpi_release (mpi);

  return TRUE;
}

gchar *
sign_challenge (gchar *text, int length, gchar *target)
{
  char hash[20];
  gchar *result;

  create_hash (target, text, length, hash);

  if (sign_hash (&private_key, hash, &result) == FALSE)
    return NULL;
  
  return result;
}
