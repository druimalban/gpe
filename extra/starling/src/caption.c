/* caption.c - Generate captions.
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

#define _GNU_SOURCE

#define obstack_chunk_alloc g_malloc
#define obstack_chunk_free g_free

#include <obstack.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <alloca.h>
#include <glib.h>

#include "musicdb.h"

struct chunk
{
  struct chunk *next;

  /* Offset from start of struct music_db_info.  */
  int offset;
  /* If not 0, assumes a stirng and tries to do ellipsizing.  */
  int width;
  /* The format string.  It contains exactly one format specified.  */
  char fmt[0];
};

struct caption
{
  struct obstack pool;

  /* Fields we need when calling music_db_get_info.  */
  int fields;

  struct chunk *chunks;
};

struct my_info
{
  struct music_db_info db;
  int seconds;
  int minutes;
};

struct caption *
caption_create (const char *fmt)
{
  struct caption *caption = g_malloc (sizeof (*caption));
  caption->fields = 0;

  obstack_init (&caption->pool);

  obstack_blank (&caption->pool, sizeof (struct chunk));
  struct chunk *f = obstack_base (&caption->pool);
  f->next = NULL;
  f->offset = -1;
  f->width = 0;

  struct chunk **nextp = &caption->chunks;

  const char *c;
  for (c = fmt; *c; c ++)
    {
      if (*c == '%')
	{
	  c ++;

	  f = obstack_base (&caption->pool);
	  if (f->offset != -1)
	    /* Need a new one.  */
	    {
	      /* Finish the last object and update the next pointer.  */
	      obstack_1grow (&caption->pool, 0);
	      f = obstack_finish (&caption->pool);
	      *nextp = f;
	      nextp = &f->next;

	      /* Start the next object.  */
	      obstack_blank (&caption->pool, sizeof (struct chunk));
	      f = obstack_base (&caption->pool);
	      f->next = NULL;
	      f->offset = -1;
	      f->width = 0;
	    }

	  char *end;
	  int w = strtol (c, &end, 0);
	  int p = 0;
	  if (*end == '.')
	    {
	      char *c2 = end + 1;
	      p = strtol (c2, &end, 0);
	    }

	  char fmt_char;
	  switch (*end)
	    {
	    case 'a':
	      /* Artist.  */
	      caption->fields |= MDB_ARTIST;
	      f->offset = offsetof (struct music_db_info, artist);
	      fmt_char = 's';
	      break;
	    case 'A':
	      /* Album.  */
	      caption->fields |= MDB_ALBUM;
	      f->offset = offsetof (struct music_db_info, album);
	      fmt_char = 's';
	      break;
	    case 't':
	      /* Title.  */
	      caption->fields |= MDB_TITLE | MDB_SOURCE;
	      f->offset = offsetof (struct music_db_info, title);
	      fmt_char = 's';
	      break;
	    case 'T':
	      /* Track.  */
	      caption->fields |= MDB_TRACK;
	      f->offset = offsetof (struct music_db_info, track);
	      fmt_char = 'd';
	      break;
	    case 'g':
	      /* Genre.  */
	      caption->fields |= MDB_GENRE;
	      f->offset = offsetof (struct music_db_info, genre);
	      fmt_char = 's';
	      break;
	    case 'r':
	      /* Rating.  */
	      caption->fields |= MDB_RATING;
	      f->offset = offsetof (struct music_db_info, rating);
	      fmt_char = 'd';
	      break;
	    case 'd':
	      /* Duration.  */
	      caption->fields |= MDB_DURATION;
	      f->offset = offsetof (struct music_db_info, duration);
	      fmt_char = 'd';
	      break;
	    case 's':
	      /* Duration % 60.  */
	      caption->fields |= MDB_DURATION;
	      f->offset = offsetof (struct my_info, seconds);
	      fmt_char = 'd';
	      break;
	    case 'm':
	      /* Minutes.  */
	      caption->fields |= MDB_DURATION;
	      f->offset = offsetof (struct my_info, minutes);
	      fmt_char = 'd';
	      break;
	    case 'c':
	      /* Play count.  */
	      caption->fields |= MDB_PLAY_COUNT;
	      f->offset = offsetof (struct music_db_info, play_count);
	      fmt_char = 'd';
	      break;

	    case '%':
	      if (end == c + 1)
		{
		  obstack_1grow (&caption->pool, '%');
		  obstack_1grow (&caption->pool, '%');
		}
	      else
	    default:
		obstack_grow (&caption->pool, c, end - c + 1);
	      /* The +1 will be added by the for loop.  */
	      c = end;
	      continue;
	    }

	  obstack_1grow (&caption->pool, '%');
	  obstack_grow (&caption->pool, c, end - c);

	  if (p && fmt_char == 's')
	    {
	      f = obstack_base (&caption->pool);
	      f->width = p;
	    }
	  obstack_1grow (&caption->pool, fmt_char);

	  /* The +1 will be added by the for loop.  */
	  c = end;
	}
      else
	obstack_1grow (&caption->pool, *c);
    }
  obstack_1grow (&caption->pool, 0);
  *nextp = obstack_finish (&caption->pool);

  f = *nextp;

  return caption;
}

void
caption_free (struct caption *caption)
{
  obstack_free (&caption->pool, NULL);
  g_free (caption);
}

char *
caption_render (struct caption *caption, MusicDB *db, int uid)
{
  struct my_info info;
  memset (&info, 0, sizeof (info));
  info.db.fields = caption->fields;
  music_db_get_info (db, uid, &info.db);

  char *unknown = "unknown";
  if ((info.db.fields & MDB_ARTIST) && ! info.db.artist)
    info.db.artist = unknown;
  if ((info.db.fields & MDB_ALBUM) && ! info.db.album)
    info.db.album = unknown;
  if ((info.db.fields & MDB_GENRE) && ! info.db.genre)
    info.db.genre = unknown;

  /* Treat title specially.  If it is NULL, then use the source.  */
  int free_title = true;
  if ((info.db.fields & MDB_TITLE) && ! info.db.title)
    {
      if (info.db.source)
	{
	  int i;
	  int len = strlen (info.db.source);
	  int slashes = 0;
	  for (i = len - 1; i >= 0; i --)
	    if (info.db.source[i] == '/')
	      {
		slashes ++;
		if (slashes == 3)
		  break;
	      }

	  if (info.db.source[i] == '/')
	    i ++;
	  info.db.title = &info.db.source[i];
	}
      else
	info.db.title = unknown;
      free_title = false;
    }

  if ((info.db.fields & MDB_DURATION))
    {
      info.seconds = info.db.duration % 60;
      info.minutes = info.db.duration / 60;
    }

  struct chunk *c;
  for (c = caption->chunks; c; c = c->next)
    {
      if (c->offset != -1)
	{
	  char *s = * (void **) ((void *) &info + c->offset);
	  char *alt = NULL;

	  if (c->width && strlen (s) > c->width - 3)
	    asprintf (&alt, "%.*s...", c->width - 3, s);

	  obstack_printf (&caption->pool, c->fmt, alt ?: s);

	  g_free (alt);
	}
      else if (c->offset != -1)
	obstack_printf (&caption->pool,
			c->fmt, * (void **) ((void *) &info + c->offset));
      else
	obstack_printf (&caption->pool, "%s", c->fmt);
    }

  g_free (info.db.source);
  if (info.db.artist != unknown)
    g_free (info.db.artist);
  if (info.db.album != unknown)
    g_free (info.db.album);
  if (info.db.genre != unknown)
    g_free (info.db.genre);

  if (free_title)
    g_free (info.db.title);

  obstack_1grow (&caption->pool, 0);
  char *result = obstack_finish (&caption->pool);
  char *copy = g_strdup (result);
  obstack_free (&caption->pool, result);

  return copy;
}
