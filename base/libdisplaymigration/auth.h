/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef AUTH_H
#define AUTH_H

#include <gcrypt.h>
#include <glib.h>

struct rsa_key
{
  GcryMPI n, e, d, p, q, u;
};

extern void update_challenge (void);
extern void generate_challenge (void);
extern gboolean check_rsa_sig (char *display, char *data);

extern GcryMPI mpi_from_sexp (GcrySexp r, char *tag);
extern char *hex_from_mpi (GcryMPI m);

#endif
