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

#include "auth.h"

#define CHALLENGE_LEN 64

char challenge_string[(CHALLENGE_LEN * 2) + 9];

static unsigned char challenge_bytes[CHALLENGE_LEN];
static unsigned long challenge_seq;

#define hexbyte(a)  (((a) > 10) ? (a) + 'a' - 10 : (a) + '0')

void 
update_challenge(void)
{
  int i;
  unsigned char *p = challenge_string;

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

gchar *
form_message (char *display, char *method, char *challenge)
{
  return g_strdup_printf ("%s %s %s", display, method, challenge);
}
