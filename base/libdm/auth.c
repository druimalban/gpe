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

#include "auth.h"

#define CHALLENGE_LEN 64

gchar *challenge_string;

static unsigned char challenge_bytes[CHALLENGE_LEN];
static unsigned long challenge_seq;

#define hexbyte(a)  (((a) >= 10) ? (a) + 'a' - 10 : (a) + '0')

void 
update_challenge (void)
{
  int i;
  unsigned char *p;

  if (challenge_string == NULL)
    challenge_string = g_malloc ((CHALLENGE_LEN * 2) + 9);

  p = challenge_string;

  for (i = 0; i < CHALLENGE_LEN; i++)
    {
      *p++ = hexbyte (challenge_bytes[i] >> 4);
      *p++ = hexbyte (challenge_bytes[i] & 15);
    }
  
  sprintf (p, "%08lx", challenge_seq++);
}

void
generate_challenge (void)
{
  gcry_randomize (challenge_bytes, sizeof (challenge_bytes), GCRY_STRONG_RANDOM);
  update_challenge ();
}

void
create_hash (char *display, char *challenge, size_t len, char *result)
{
  size_t dlen = strlen (display);
  gchar *buf = g_malloc (dlen + 1 + len);
  strcpy (buf, display);
  memcpy (buf + dlen + 1, challenge, len);
  gcry_md_hash_buffer (GCRY_MD_SHA1, result, buf, len + dlen + 1);
  g_free (buf);
}

gboolean
check_signature (struct rsa_key *k, char *hash, char *sigbuf)
{
  GcryMPI mpi, mpi2;
  GcrySexp data, sig, key;
  size_t nb = 19;
  int rc;

  gcry_mpi_scan (&mpi, GCRYMPI_FMT_USG, hash, &nb);

  gcry_sexp_build (&data, NULL, "(data (flags pkcs1) (hash sha1 %m))", mpi);
  
  gcry_mpi_release (mpi);

  gcry_sexp_build (&key, NULL, "(public-key (rsa (n %m) (e %m)))",
		   k->n, k->e, k->d, k->p, k->q, k->u);

  gcry_mpi_scan (&mpi2, GCRYMPI_FMT_HEX, sigbuf, NULL);

  gcry_sexp_build (&sig, NULL, "(sig-val (rsa (s %m)))", mpi2);

  rc = gcry_pk_verify (sig, data, key);

  gcry_sexp_release (data);
  gcry_sexp_release (key);
  gcry_sexp_release (sig);
  gcry_mpi_release (mpi);
  gcry_mpi_release (mpi2);

  if (rc)
    return FALSE;

  return TRUE;
}

static struct rsa_key *
parse_pubkey (char *s)
{
  struct rsa_key *r;
  GcryMPI n, e;
  char *p = strchr (s, ' ');
  if (p == NULL)
    return NULL;
  *p++ = 0;
  
  gcry_mpi_scan (&e, GCRYMPI_FMT_HEX, s, NULL);
  gcry_mpi_scan (&n, GCRYMPI_FMT_HEX, p, NULL);

  r = g_malloc0 (sizeof (struct rsa_key));
  r->e = e;
  r->n = n;
  return r;
}

static struct rsa_key *
lookup_pubkey (u_int32_t id)
{
  const gchar *home_dir = g_get_home_dir ();
  gchar *filename = g_strdup_printf ("%s/.gpe/migrate/public", home_dir);
  FILE *fp = fopen (filename, "r");
  struct rsa_key *r = NULL;

  if (fp)
    {
      while (!feof (fp))
	{
	  char buffer[4096];
	  if (fgets (buffer, 4096, fp))
	    {
	      char *p;
	      u_int32_t this_id = strtoul (buffer, &p, 16);
	      if (p != buffer && *p == ' ')
		{
#ifdef DEBUG
		  fprintf (stderr, "found id %x\n", this_id);
#endif
		  if (this_id == id)
		    {
		      r = parse_pubkey (++p);
		      break;
		    }
		}
	    }
	}
      fclose (fp);
    }

  g_free (filename);
  return r;
}

static void
free_pubkey (struct rsa_key *k)
{
  gcry_mpi_release (k->n);
  gcry_mpi_release (k->e);

  g_free (k);
}

gboolean
check_rsa_sig (char *display, char *data)
{
  u_int32_t key_id;
  char *ep;
  char *p;
  struct rsa_key *k;
  char hash[20];
  gboolean rc;

  p = strchr (data, ' ');
  if (p == NULL)
    return FALSE;
  *p++ = 0;

  key_id = strtoul (data, &ep, 16);
  if (*ep)
    return FALSE;

#ifdef DEBUG
  fprintf (stderr, "using key id %x\n", key_id);
#endif

  k = lookup_pubkey (key_id);
  if (k == NULL)
    return FALSE;

  create_hash (display, challenge_string, strlen (challenge_string), hash);

  rc = check_signature (k, hash, p);

  free_pubkey (k);

  return rc;
}
