/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef DISPLAYMIGRATION_AUTH_H
#define DISPLAYMIGRATION_AUTH_H

#include <gcrypt.h>
#include <glib.h>

extern void displaymigration_auth_update_challenge (void);
extern void displaymigration_auth_generate_challenge (void);

extern gboolean displaymigration_auth_validate_request (char *display, char *data);

extern gchar *displaymigration_auth_challenge_string;

#endif
