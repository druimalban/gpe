/* cache.h - Simple cache manager interface.
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

#ifndef __STARLING_CACHE_H
#define __STARLING_CACHE_H

#define SC_LINES 256
#define SC_WAYS 2

struct simple_cache
{
  unsigned int timestamps[SC_LINES / SC_WAYS][SC_WAYS];
  int keys[SC_LINES / SC_WAYS][SC_WAYS];
  void *objects[SC_LINES / SC_WAYS][SC_WAYS];

  /* Called when evicting an object from the cache.  */
  void (*object_evict) (void *object);
};

/* Create a new simple cache.  */
void simple_cache_init (struct simple_cache *sc,
			void (*object_evict) (void *object));

/* Destroy the cache, dropping all objects.  */
void simple_cache_deinit (struct simple_cache *sc);

/* Add the object OBJECT referenced by key KEY to the cache SC.  An
   object with key must not already exists.  */
void simple_cache_add (struct simple_cache *sc, int key, void *object);

/* Return the object with key KEY (if any).  */
void *simple_cache_find (struct simple_cache *sc, int key);

/* Drop all elements in the cache.  */
void simple_cache_drop (struct simple_cache *sc);

/* Drop the element with key KEY (if any).  */
void simple_cache_shootdown (struct simple_cache *sc, int key);

#endif /* __STARLING_CACHE_H  */
