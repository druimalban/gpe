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
  struct chunk *cond_true;
  struct chunk *cond_false;

  /* Offset from start of struct music_db_info.  */
  int offset;
  /* The number of bytes in the field.  */
  int bytes;

  /* If not 0, assumes a stirng and tries to do ellipsizing.  */
  int width;
  /* The format string.  It contains exactly one format specifier.  */
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

static struct chunk *
build (struct caption *caption, const char *fmt)
{
  struct chunk *head = NULL;
  struct chunk **nextp = &head;

  /* Finish the extant chunk.  */
  struct chunk *finish_chunk (void)
  {
    /* A trailing NUL.  */
    obstack_1grow (&caption->pool, 0);
    struct chunk *chunk = obstack_finish (&caption->pool);

    /* Update the previous chunk's next pointer.  */
    *nextp = chunk;
    nextp = &chunk->next;

    return chunk;
  }

  /* Start a new chunk.  */
  struct chunk *start_chunk (void)
  {
    obstack_blank (&caption->pool, sizeof (struct chunk));

    struct chunk *chunk = obstack_base (&caption->pool);
    memset (chunk, 0, sizeof (*chunk));
    chunk->offset = -1;
    return chunk;
  }

  struct chunk *new_chunk (void)
  {
    if (obstack_object_size (&caption->pool) == sizeof (struct chunk))
      /* Nothing has been added to this chunk.  */
      return;

    finish_chunk ();
    return start_chunk ();
  }

  start_chunk ();

  const char *c = fmt;
  while (*c)
    {
      if (*c == '\\')
	{
	  obstack_1grow (&caption->pool, *c);
	  c ++;
	  if (*c)
	    {
	      obstack_1grow (&caption->pool, *c);
	      c ++;
	    }
	}
      else if (*c != '%')
	{
	  obstack_1grow (&caption->pool, *c);
	  c ++;
	}
      else
	{
	  c ++;

	  struct chunk *chunk = obstack_base (&caption->pool);
	  if (chunk->offset != -1)
	    /* This chunk already contains a parameter.  As we allow
	       zero or one parameters per chunk, start a new one.  */
	    chunk = new_chunk ();

	  /* Determine the width and precision (if any).  */
	  char *end;
	  int w = strtol (c, &end, 0);
	  int p = 0;
	  char *period = NULL;
	  if (*end == '.')
	    {
	      period = end;
	      char *c2 = end + 1;
	      p = strtol (c2, &end, 0);
	    }

	  char fmt_char;
	  switch (*end)
	    {
	    case 'u':
	      /* Source.  */
	      caption->fields |= MDB_SOURCE;
	      chunk->offset = offsetof (struct music_db_info, source);
	      fmt_char = 's';
	      break;
	    case 'a':
	      /* Artist.  */
	      caption->fields |= MDB_ARTIST;
	      chunk->offset = offsetof (struct music_db_info, artist);
	      fmt_char = 's';
	      break;
	    case 'A':
	      /* Album.  */
	      caption->fields |= MDB_ALBUM;
	      chunk->offset = offsetof (struct music_db_info, album);
	      fmt_char = 's';
	      break;
	    case 't':
	      /* Title.  */
	      caption->fields |= MDB_TITLE;
	      chunk->offset = offsetof (struct music_db_info, title);
	      fmt_char = 's';
	      break;
	    case 'T':
	      /* Track.  */
	      caption->fields |= MDB_TRACK;
	      chunk->offset = offsetof (struct music_db_info, track);
	      fmt_char = 'd';
	      break;
	    case 'g':
	      /* Genre.  */
	      caption->fields |= MDB_GENRE;
	      chunk->offset = offsetof (struct music_db_info, genre);
	      fmt_char = 's';
	      break;
	    case 'r':
	      /* Rating.  */
	      caption->fields |= MDB_RATING;
	      chunk->offset = offsetof (struct music_db_info, rating);
	      fmt_char = 'd';
	      break;
	    case 'd':
	      /* Duration.  */
	      caption->fields |= MDB_DURATION;
	      chunk->offset = offsetof (struct music_db_info, duration);
	      fmt_char = 'd';
	      break;
	    case 's':
	      /* Duration % 60.  */
	      caption->fields |= MDB_DURATION;
	      chunk->offset = offsetof (struct my_info, seconds);
	      fmt_char = 'd';
	      break;
	    case 'm':
	      /* Minutes.  */
	      caption->fields |= MDB_DURATION;
	      chunk->offset = offsetof (struct my_info, minutes);
	      fmt_char = 'd';
	      break;
	    case 'c':
	      /* Play count.  */
	      caption->fields |= MDB_PLAY_COUNT;
	      chunk->offset = offsetof (struct music_db_info, play_count);
	      fmt_char = 'd';
	      break;

	    case '%':
	      obstack_1grow (&caption->pool, '%');
	      if (end == c + 1)
		{
		  obstack_1grow (&caption->pool, '%');
		  c = end;
		}
	      else
		/* There were number between the two %'s, e.g., %10%s.
		   Print everything literally up to but not including
		   the second %.  Continue processing with the second
		   percent.  */
		{
		  obstack_grow (&caption->pool, c, end - c + 1 - 1);
		  c = end;
		}
	      continue;

	    default:
		/* Unknown specifier.  Prefix the leading % with a %
		   (so that a % is printed and printf doesn't try to
		   read a parameter that is not there), and print the
		   rest literally.  */
	      obstack_1grow (&caption->pool, '%');
	      obstack_grow (&caption->pool, c, end - c + 1);
	      c = end + 1;
	      continue;
	    }

	  if (fmt_char == 'd')
	    chunk->bytes = sizeof (int);
	  else
	    chunk->bytes = sizeof (void *);

	  if ((end[1] == '?' && end[2] == '(')
	      || (end[1] == '?' && end[2] == '!' && end[3] == '('))
	    /* It's a condition: "%s?(...)(...)".  */
	    {
	      bool neg = end[2] == '!';

	      end = strchr (end, '(') + 1;

	      /* Find the matching right paren.  We cannot just use
		 strchr as it might be nested.  START should point one
		 beyond the left paren.  Returns the location of the
		 right paren or the end of the string, if there is
		 none.  */
	      char *find_right_paren (char *start)
	      {
		char *end = start;

		int depth = 1;
		while (*end)
		  {
		    end += strcspn (end, "\\()");
		    switch (end[0])
		      {
		      case '\\':
			/* Ignore the next character.  */
			if (end[1] == 0)
			  /* Whoops!  End of string.  */
			  return &end[1];
			end ++;
			break;

		      case '(':
			depth ++;
			break;

		      case ')':
			depth --;
			if (depth == 0)
			  return end;
		      }

		    /* We are pointing at the stop character, advance
		       one.  */
		    end ++;
		  }

		return end;
	      }

	      char *cond_first_start = end;
	      char *cond_first_end = find_right_paren (cond_first_start);
	      g_assert (cond_first_end[0] == ')' || cond_first_end[0] == 0);

	      char *cond_sec_start = NULL;
	      char *cond_sec_end = NULL;
	      if (cond_first_end[0] && cond_first_end[1] == '(')
		/* There is a false branch.  */
		{
		  cond_sec_start = &cond_first_end[2];
		  cond_sec_end = find_right_paren (cond_sec_start);
		}

	      /* Update the END pointer.  */
	      if (cond_sec_end)
		{
		  if (*cond_sec_end)
		    c = cond_sec_end + 1;
		  else
		    c = cond_sec_end;
		}
	      else
		{
		  if (*cond_first_end)
		    c = cond_first_end + 1;
		  else
		    c = cond_first_end;
		}

	      /* Build the conditionals.  */
	      chunk = finish_chunk ();

	      *cond_first_end = 0;
	      struct chunk *first = build (caption, cond_first_start);

	      struct chunk *sec = NULL;
	      if (cond_sec_end)
		{
		  *cond_sec_end = 0;
		  sec = build (caption, cond_sec_start);
		}

	      chunk->cond_true = neg ? sec : first;
	      chunk->cond_false = neg ? first : sec;

	      start_chunk ();
	    }
	  else
	    /* Not a conditional.  */
	    {
	      if (p && fmt_char == 's')
		/* Has a precision and is a string.  */
		{
		  chunk->width = p;

		  /* Everything from the % until, but not and
		     including, the period.  */
		  obstack_grow (&caption->pool, c - 1, period - c + 1);
		  if (p < 0)
		    obstack_printf (&caption->pool, "s%%s");
		  else
		    obstack_printf (&caption->pool, ".*s%%s");
		}
	      else
		{
		  /* Everything from the % until, but not including, the
		     format character.  */
		  obstack_grow (&caption->pool, c - 1, end - c + 1);
		  obstack_1grow (&caption->pool, fmt_char);
		}

	      c = end + 1;
	    }
	}
    }

  /* Finish the last chunk.  */
  finish_chunk ();

  /* Return the head.  */
  return head;
}

struct caption *
caption_create (const char *fmt)
{
  struct caption *caption = g_malloc (sizeof (*caption));
  caption->fields = 0;
  obstack_init (&caption->pool);

  /* Make a copy of FMT as build needs to modify it.  */
  int len = strlen (fmt);
  char *f = alloca (len + 1);
  memcpy (f, fmt, len + 1);

  caption->chunks = build (caption, f);

  return caption;
}

void
caption_free (struct caption *caption)
{
  obstack_free (&caption->pool, NULL);
  g_free (caption);
}

static void
render (struct caption *caption, struct my_info *info, struct chunk *c)
{
  for (; c; c = c->next)
    if (c->offset == -1)
      obstack_printf (&caption->pool, "%s", c->fmt);
    else
      {
	g_assert (c->offset >= 0);
	g_assert (c->offset + c->bytes <= sizeof (*info));

	void *locp = (void *) info + c->offset;

	if (c->cond_true)
	  /* This chunk represents a condition.  */
	  {
	    /* The format consists of literals.  */
	    obstack_printf (&caption->pool, "%s", c->fmt);

	    int *i;
	    for (i = locp; (void *) i < locp + c->bytes; i ++)
	      if (*i)
		break;

	    if ((void *) i < locp + c->bytes)
	      /* Non-zero.  Execute the true branch.  */
	      render (caption, info, c->cond_true);
	    else if (c->cond_false)
	      /* Zero.  Execute the false branch.  */
	      render (caption, info, c->cond_false);
	  }
	else if (c->width != 0)
	  /* This chunk prints a string, which we may need to
	     ellipsize.  */
	  {
	    char *s = * (char **) locp;

	    int p = c->width > 0 ? c->width : - c->width;
	    int len = strlen (s);
	    bool too_long = len > p - 2;

	    if (too_long)
	      {
		if (c->width < 0)
		  obstack_printf (&caption->pool,
				  c->fmt, "...", s + len - p + 2);
		else
		  obstack_printf (&caption->pool,
				  c->fmt, p - 2, s, "...");
	      }
	    else
	      {
		if (c->width < 0)
		  obstack_printf (&caption->pool, c->fmt, "", s);
		else
		  obstack_printf (&caption->pool, c->fmt, c->width, s, "");
	      }
	  }
	else
	  {
	    if (c->bytes == sizeof (char *))
	      obstack_printf (&caption->pool, c->fmt, * (char **) locp);
	    else
	      {
		g_assert (c->bytes == sizeof (int));
		obstack_printf (&caption->pool, c->fmt, * (int *) locp);
	      }
	  }
      }
}

char *
caption_render (struct caption *caption, MusicDB *db, int uid)
{
  struct my_info info;
  memset (&info, 0, sizeof (info));
  info.db.fields = caption->fields;
  music_db_get_info (db, uid, &info.db);

  if ((info.db.fields & MDB_DURATION))
    {
      info.seconds = info.db.duration % 60;
      info.minutes = info.db.duration / 60;
    }

  render (caption, &info, caption->chunks);

  g_free (info.db.source);
  g_free (info.db.artist);
  g_free (info.db.album);
  g_free (info.db.title);
  g_free (info.db.genre);

  obstack_1grow (&caption->pool, 0);
  char *result = obstack_finish (&caption->pool);
  char *copy = g_strdup (result);
  obstack_free (&caption->pool, result);

  return copy;
}
