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

#include <libdisplaymigration/auth.h>
#include <libdisplaymigration/crypt.h>

#include "keygen.h"

static struct rsa_key private_key;
static u_int32_t key_id;

gchar *
sign_challenge (gchar *text, int length, gchar *target)
{
  char hash[20];
  gchar *sig, *result;

  memset (hash, 0, sizeof (hash));

  displaymigration_crypt_create_hash (target, text, length, hash);
  if (displaymigration_crypt_sign_hash (&private_key, hash, &sig) == FALSE)
    return NULL;

  result = g_strdup_printf ("%08x %s", key_id, sig);
  g_free (sig);

  return result;
}

static gboolean
parse_key (char *s, struct rsa_key *r)
{
  gcry_mpi_t n, e, d, p, q, u;
  char *sp;

  sp = strtok (s, " \n");
  key_id = strtoul (sp, NULL, 16);
  sp = strtok (NULL, " \n");
  gcry_mpi_scan (&e, GCRYMPI_FMT_HEX, sp, 0, NULL);
  sp = strtok (NULL, " \n");
  gcry_mpi_scan (&d, GCRYMPI_FMT_HEX, sp, 0, NULL);
  sp = strtok (NULL, " \n");
  gcry_mpi_scan (&n, GCRYMPI_FMT_HEX, sp, 0, NULL);
  sp = strtok (NULL, " \n");
  gcry_mpi_scan (&p, GCRYMPI_FMT_HEX, sp, 0, NULL);
  sp = strtok (NULL, " \n");
  gcry_mpi_scan (&q, GCRYMPI_FMT_HEX, sp, 0, NULL);
  sp = strtok (NULL, " \n");
  gcry_mpi_scan (&u, GCRYMPI_FMT_HEX, sp, 0, NULL);

  r->e = e;
  r->d = d;
  r->n = n;
  r->p = p;
  r->q = q;
  r->u = u;

  return TRUE;
}

void
crypt_init (void)
{
  gchar *filename;
  FILE *fp;
  gboolean key_found = FALSE;

  gcry_control (GCRYCTL_INIT_SECMEM, 1);
  gcry_check_version (NULL);

  memset (&private_key, 0, sizeof (private_key));

  home_dir = g_get_home_dir ();
  filename = g_strdup_printf ("%s/.gpe/migrate/secret", home_dir);
  fp = fopen (filename, "r");

  if (fp)
    {
      char buffer[4096];
      if (fgets (buffer, 4096, fp) && parse_key (buffer, &private_key))
	key_found = TRUE;
      fclose (fp);
    }

  g_free (filename);

  if (! key_found)
    {
      generate_key (&private_key);
      write_public (&private_key);
      write_secret (&private_key);

      key_id = private_key_id (&private_key);
    }
}
