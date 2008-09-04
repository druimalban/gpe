/* cache.c - Simple cache manager.
   Copyright (C) 2008 Neal H. Walfield <neal@walfield.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see
   <http://www.gnu.org/licenses/>.  */

#include "cache.h"

#include <stdlib.h>
#include <assert.h>

#define INVALID_KEY -1

static unsigned int ts_counter;

void
simple_cache_init (struct simple_cache *sc,
		   void (*object_evict) (void *object))
{
  int i, j;
  for (i = 0; i < SC_LINES / SC_WAYS; i ++)
    for (j = 0; j < SC_WAYS; j ++)
      sc->keys[i][j] = INVALID_KEY;

  sc->object_evict = object_evict;
}

void
simple_cache_deinit (struct simple_cache *sc)
{
  simple_cache_drop (sc);
}

void
simple_cache_add (struct simple_cache *sc, int key, void *object)
{
  assert (key != INVALID_KEY);

  int idx = key % (SC_LINES / SC_WAYS);

  int i;
  for (i = 0; i < SC_WAYS; i ++)
    if (sc->keys[idx][i] == INVALID_KEY)
      goto done;

  /* All slots are filled.  Find the least recently used one.  */
  int min = 0;
  int ts = sc->timestamps[idx][0];
  for (i = 1; i < SC_WAYS; i ++)
    if (sc->timestamps[idx][i] < ts)
      {
	ts = sc->timestamps[idx][i];
	min = i;
      }
  i = min;

  if (sc->object_evict)
    sc->object_evict (sc->objects[idx][i]);

 done:
  sc->keys[idx][i] = key;
  sc->objects[idx][i] = object;
  sc->timestamps[idx][i] = ts_counter ++;
}

void *
simple_cache_find (struct simple_cache *sc, int key)
{
  assert (key != INVALID_KEY);

  int idx = key % (SC_LINES / SC_WAYS);

  int i;
  for (i = 0; i < SC_WAYS; i ++)
    if (sc->keys[idx][i] == key)
      {
	sc->timestamps[idx][i] = ts_counter ++;
	return sc->objects[idx][i];
      }

  return NULL;
}

void
simple_cache_drop (struct simple_cache *sc)
{
  int i, j;
  for (i = 0; i < SC_LINES / SC_WAYS; i ++)
    for (j = 0; j < SC_WAYS; j ++)
      if (sc->keys[i][j] != INVALID_KEY)
	{
	  if (sc->object_evict)
	    sc->object_evict (sc->objects[i][j]);

	  sc->keys[i][j] = INVALID_KEY;
	}
}

void
simple_cache_shootdown (struct simple_cache *sc, int key)
{
  int idx = key % (SC_LINES / SC_WAYS);

  int i;
  for (i = 0; i < SC_WAYS; i ++)
    if (sc->keys[idx][i] == key)
      {
	if (sc->object_evict)
	  sc->object_evict (sc->objects[idx][i]);

	/* Invalidate the entry.  */
	sc->keys[idx][i] = INVALID_KEY;

	return;
      }
}
