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

#include "libdisplaymigration/auth.h"
#include "libdisplaymigration/crypt.h"

#define CHALLENGE_LEN 64

gchar *displaymigration_auth_challenge_string;

static unsigned char challenge_bytes[CHALLENGE_LEN];
static unsigned long challenge_seq;

#define hexbyte(x)  ((x) >= 10 ? (x) + 'a' - 10 : (x) + '0')

void 
displaymigration_auth_update_challenge (void)
{
  int i;
  unsigned char *p;

  if (displaymigration_auth_challenge_string == NULL)
    displaymigration_auth_challenge_string = g_malloc ((CHALLENGE_LEN * 2) + 9);

  p = displaymigration_auth_challenge_string;

  for (i = 0; i < CHALLENGE_LEN; i++)
    {
      *p++ = hexbyte (challenge_bytes[i] >> 4);
      *p++ = hexbyte (challenge_bytes[i] & 15);
    }
  
  sprintf (p, "%08lx", challenge_seq++);
}

void
displaymigration_auth_generate_challenge (void)
{
  gcry_randomize (challenge_bytes, sizeof (challenge_bytes), GCRY_STRONG_RANDOM);
  displaymigration_auth_update_challenge ();
}

static struct rsa_key *
parse_pubkey (char *s)
{
  struct rsa_key *r;
  gcry_mpi_t n, e;
  gchar *sp;

  sp = strtok (s, " \n");
  gcry_mpi_scan (&e, GCRYMPI_FMT_HEX, sp, 0, NULL);
  sp = strtok (NULL, " \n");
  gcry_mpi_scan (&n, GCRYMPI_FMT_HEX, sp, 0, NULL);

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
displaymigration_auth_validate_request (char *display, char *data)
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

  k = lookup_pubkey (key_id);
  if (k == NULL)
    return FALSE;

  displaymigration_crypt_create_hash (display, displaymigration_auth_challenge_string, 
			   strlen (displaymigration_auth_challenge_string), hash);

  rc = displaymigration_crypt_check_signature (k, hash, p);

  free_pubkey (k);

  return rc;
}
