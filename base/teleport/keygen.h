/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef KEYGEN_H
#define KEYGEN_H

#include "crypt.h"

extern gchar *home_dir;

extern int generate_key (struct rsa_key *rv);
extern gboolean write_public (struct rsa_key *k);
extern gboolean write_secret (struct rsa_key *k);
extern unsigned long private_key_id (struct rsa_key *k);

#endif
