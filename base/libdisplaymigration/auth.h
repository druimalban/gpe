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

struct rsa_key
{
  u_int32_t id;

  GcryMPI n, e, d, p, q, u;
};

extern void update_challenge (void);
extern void generate_challenge (void);

#endif
