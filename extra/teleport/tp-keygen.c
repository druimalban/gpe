/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>

#include <glib.h>
#include <gcrypt.h>
#include <sys/stat.h>

#include "keygen.h"

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
