/*

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

#include "hash.h"

// static struct nlist *hashtab[HASHSIZE];

struct hash*
hash_new(int size)
{
  struct hash *h = (struct hash *)(malloc(sizeof(struct hash)));
  h->size = size;
  h->hashtab = (struct nlist **)(malloc(sizeof(struct nlist)*size));
  memset((void *)h->hashtab, 0, sizeof(struct nlist)*size);
  return h;
}

unsigned int hashfunc(struct hash *h, char *s)
{
   unsigned int hashval;

   for(hashval = 0; *s != '\0'; s++)
      hashval = *s + 21 * hashval;
   return hashval % h->size;
}

struct nlist *hash_lookup(struct hash *h, char *s)
{
   struct nlist *np;

   for (np = h->hashtab[hashfunc(h, s)]; np != NULL; np = np->next)
      if (strcmp(s, np->key) == 0)
	 return np;
   return NULL;
}

struct nlist *hash_add(struct hash *h, char *key, char *val)
{
   struct nlist *np;
   unsigned int hashval;

   if ((np = hash_lookup(h, key)) == NULL)
   {
      np = (struct nlist * ) malloc(sizeof(*np));
      if ( np == NULL || (np->key = strdup(key)) == NULL)
	 return NULL;
      hashval = hashfunc(h, key);
      np->next = h->hashtab[hashval];
      h->hashtab[hashval] = np;
   } else {
      free((void *) np->value);
   }
   if ((np->value = strdup(val)) == NULL)
      return NULL;
   return np;
}

void hash_empty(struct hash *h)
{
   memset(h->hashtab, 0, sizeof(h->hashtab));
}

void 
hash_destroy(struct hash *h)
{
  free(h->hashtab);
  free(h);
}
