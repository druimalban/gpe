/* starling-catalog.c - Starling catalog.
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

#include "musicdb.h"
#include "config.h"
#include "caption.h"

#define CONFIG_DATABASE ".starling/playlist"

#include <stdio.h>
#define obstack_chunk_alloc g_malloc
#define obstack_chunk_free g_free
#include <obstack.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <argp.h>

const char *argp_program_version = PACKAGE_VERSION;
const char *argp_program_bug_address = PACKAGE_BUGREPORT;

static char doc[]
  = "starling-catalog -- a command-line front-end for querying starling's \
database\v\
FORMAT\n\
\n\
  The support format characters are:\n\
\n\
     %a - artist\n\
     %A - album\n\
     %t - title or, if NULL, source\n\
     %T - track\n\
     %g - genre\n\
     %r - rating\n\
     %d - duration\n\
     %c - play count\n\
\n\
  The following format options are support:\n\
\n\
    %[width][.][precision]c - width and precision.\n\
\n\
QUERY\n\
\n\
A query is composed of zero or more search terms.  By default, the \
keywords are searched for (case insensitively) in a track's title, \
album, artist, genre and source fields. If a keyword is a substring \
of any of those fields, the track is considered to match and is \
shown. It is possible to search for a specific field by prefixing \
the keyword with the field of interest followed by a colon. For \
instance, to find tracks by the BBC, one could use artist:bbc.\n\
\n\
Multiple keywords may be given. By default, only those tracks that \
match all keywords are shown. (That is, the intersection is taken.) \
By separating keywords by or, it is possible change this \
behavior. For instance, searching for genre:ambient or \
genre:electronic will find tracks that have the string ambient or \
the string electronic in their genre field. Likewise, not and \
parenthesis can also be used. For consistency, they keyword and is \
also recognized.\n\
\n\
Starling also supports matching against some properties of a \
track.  Starling currently supports four properties: \n\
\n\
    added - the amount of time in seconds since the track was added to \
      the data base, \n\
    played - the amount of time in seconds since the track was last \
      played, \n\
    play-count - the number of times a track has been played, and, \n\
    rating - a track's rating. \n\
\n\
To search for a property, suffix the property with a colon, followed \
by a comparison operator, either <, >, <=, >=, =, or !=, followed by \
a number. Note: spaces are not allowed. Time is measured in seconds, \
however, m, h, d, W, M, and Y, can be used to multiply by the number \
of seconds in a minute, hour, day, week, month (30 days) or year, \
respectively.\n\
\n\
To search for songs added in the last week that have not yet been \
played, one could use: \n\
\n\
      added:<1W and play-count:=0\n\
\n\
  For 4 and 5 star songs that have not been played in the last 3 days:\n\
\n\
      rating:>=4 and played:>3D\n\
\n\
To copy all songs with a rating greater than or equal to 4 to /mnt, \n\
use:\n\
\n\
    starling-catalog -0 rating:>=4 | xargs --r -I FILE -0 cp FILE /mnt";

static char args_doc[] = "[QUERY]";

static struct argp_option options[] = {
  {"database",	'd', "FILENAME",0,
   "Use FILENAME instead of the default database."},
  {"format",	'f', "FMT",	0,
   "Use FMT to format the database entries."},
  {"nul",	'0', 0,		0,
   "Terminate each entry with a NUL character instead of a newline."},
  {"playlist",	'l', "PLAYLIST",0,
   "Execute the query on PLAYLIST instead of the library."},
  { 0 }
};

static char *filename;
static char *caption;
static char sep = '\n';
static char *playlist;
static char *query;

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'd':
      filename = arg;
      break;
    case 'f':
      caption = arg;
      break;
    case '0':
      sep = 0;
      break;
    case 'l':
      playlist = arg;
      break;

    case ARGP_KEY_ARG:
      if (state->arg_num >= 2)
	/* Too many arguments. */
	argp_usage (state);

      query = arg;
      break;

    case ARGP_KEY_END:
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };

static MusicDB *db;

int
main (int argc, char *argv[])
{
  g_type_init ();

  argp_parse (&argp, argc, argv, 0, 0, NULL);

  if (! filename)
    {
      const char *home = g_get_home_dir ();
      char *dir = g_strdup_printf ("%s/.starling", home);
      filename = g_strdup_printf ("%s/" CONFIG_DATABASE, home);
      g_free (dir);
    }

  GError *err = NULL;
  db = music_db_open (filename, &err);
  if (err)
    {
      fprintf (stderr, "Failed to open %s: %s\n", filename, err->message);
      exit (1);
    }
  // g_free (filename);

  if (query && *query)
    {
      char *result;
      void *state = NULL;
      yyparse (query, &result, &state);
      query = result;
    }

  struct caption *caption_renderer = NULL;
  if (caption)
    caption_renderer = caption_create (caption);

  int callback (int uid, struct music_db_info *info)
  {
    if (caption_renderer)
      {
	char *c = caption_render (caption_renderer, db, uid);
	fputs (c, stdout);
	fputc (sep, stdout);
      }
    else
      printf ("%s%c", info->source, sep);
    return 0;
  }

  music_db_for_each (db, playlist, callback, NULL, query);

  return 0;
}
