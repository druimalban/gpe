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

extern void libdm_auth_update_challenge (void);
extern void libdm_auth_generate_challenge (void);

extern gboolean libdm_auth_validate_request (char *display, char *data);

extern gchar *libdm_auth_challenge_string;

#endif
