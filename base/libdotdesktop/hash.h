/* matchbox - a lightweight window manager

   Copyright 2002 Matthew Allum

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASHSIZE 101

struct hash {
  struct nlist **hashtab;
  int size;
};

struct nlist {
   struct nlist *next;
   char *key;
   unsigned char *value;
};

struct hash* hash_new(int size);
unsigned int hashfunc(struct hash *h, char *s);
struct nlist *hash_lookup(struct hash *h, char *s);
struct nlist *hash_add(struct hash *h, char *key, char *val);
void hash_empty();
void hash_destroy(struct hash *h);
