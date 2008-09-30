/* search.y - Search string expansion.
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

%{
#include <glib.h>
#include <sqlite.h> /* For sqlite_mprintf.  */
#include <string.h>
#include <stdlib.h>

#define YYSTYPE char *
#include "search.h"

extern int yylex (YYSTYPE *lvalp, YYLTYPE *llocp, char *search_string,
		  void **state);
extern void yyerror (YYLTYPE *llocp, char *search_string,
		     char **result, void **state, char const *error);


static int
parse_time (const char *spec)
{
  int time = 0;
  while (*spec)
    {
      char *end;
      int i = strtol (spec, &end, 10);
      switch (*end)
	{
	case 's':
	case 'S':
	  break;
	case 'm':
	  i *= 60;
	  break;
	case 'h':
	case 'H':
	  i *= 60 * 60;
	  break;
	case 0:
	case 'd':
	case 'D':
	  i *= 24 * 60 * 60;
	  break;
	case 'w':
	case 'W':
	  i *= 7 * 24 * 60 * 60;
	  break;
	case 'M':
	  i *= 30 * 24 * 60 * 60;
	  break;
	case 'y':
	case 'Y':
	  i *= 365 * 24 * 60 * 60;
	  break;
	default:
	  return time;
	}

      time += i;

      if (!*end)
	return time;
      spec = end + 1;
    }

  return time;
}

static char *
expand_term (const char *tok)
{
  const char *comparison[] = { "=", "!=", "<=", ">=", "<", ">" };
  const char *parse_comparison (const char *str, const char *def)
  {
    int i;
    for (i = 0; i < sizeof (comparison) / sizeof (comparison[0]); i ++)
      if (strncmp (comparison[i], str, strlen (comparison[i])) == 0)
	return comparison[i];
    return def;
  }

  if (strncasecmp ("added:", tok, strlen ("added:")) == 0)
    /* The time since the track was added is less/greater than
       some time ago.  */
    {
      const char *spec = tok + strlen ("added:");
      const char *comparison = parse_comparison (spec, "<");
      spec += strlen (comparison);

      return g_strdup_printf ("strftime('%%s', 'now')"
			      " - coalesce (date_added, 0) %s %d",
			      comparison, parse_time (spec));
    }
  else if (strncasecmp ("played:", tok, strlen ("played:")) == 0)
    /* The time since the track was last played is less/greater
       than some time ago.  */
    {
      const char *spec = tok + strlen ("played:");
      const char *comparison = parse_comparison (spec, "<");
      spec += strlen (comparison);

      return g_strdup_printf ("strftime('%%s', 'now')"
			      " - coalesce (date_last_played, 0) %s %d",
			      comparison, parse_time (spec));
    }
  else if (strncasecmp ("play-count:", tok, strlen ("play-count:")) == 0)
    {
      const char *spec = tok + strlen ("play-count:");
      const char *comparison = parse_comparison (spec, ">");
      spec += strlen (comparison);

      return g_strdup_printf  ("coalesce (play_count, 0) %s %d",
			       comparison, atoi (spec));
    }
  else if (strncasecmp ("artist:", tok, strlen ("artist:")) == 0
	   || strncasecmp ("album:", tok, strlen ("album:")) == 0
	   || strncasecmp ("title:", tok, strlen ("title:")) == 0
	   || strncasecmp ("genre:", tok, strlen ("genre:")) == 0
	   || strncasecmp ("source:", tok, strlen ("source:")) == 0)
    {
      const char *prefix = tok;
      char *term = strchr (tok, ':');
      g_assert (term && *term == ':');
      *term = 0;
      term ++;

      char *s = sqlite_mprintf ("%q", term);

      char *result = g_strdup_printf ("%s not null and %s like '%%%s%%'",
				      prefix, prefix, s);

      sqlite_freemem (s);

      return result;
    }
  else
    {
      char *s = sqlite_mprintf ("%q", tok);

      char *result = g_strdup_printf
	("(artist notnull and artist like '%%%s%%')"
	 " or (album notnull and album like '%%%s%%')"
	 " or (title notnull and title like '%%%s%%')"
	 " or (genre notnull and genre like '%%%s%%')"
	 " or (source notnull and source like '%%%s%%')",
	 s, s, s, s, s);

      sqlite_freemem (s);

      return result;
    }
}
%}

/* Bison declarations.  */
%left OR
%left AND
%left NOT
%token TERM

%pure-parser

%parse-param {char *search_string}
%lex-param {char *search_string}

%parse-param {char **result}

%parse-param {void **state}
%lex-param {void **state}

%%
/* Our grammar.  */
done:     exp
            {
	      *result = $1;
	    }
;

exp:      TERM
	    {
	      $$ = expand_term ($1);
	      g_free ($1);
	    }
        | exp exp
            {
	      $$ = g_strdup_printf ("(%s) and (%s)", $1, $2);
	      g_free ($1);
	      g_free ($2);
	    }
        | exp AND exp
            {
	      $$ = g_strdup_printf ("(%s) and (%s)", $1, $3);
	      g_free ($1);
	      g_free ($3);
	    }
        | exp OR exp
            {
	      $$ = g_strdup_printf ("(%s) or (%s)", $1, $3);
	      g_free ($1);
	      g_free ($3);
	    }
        | NOT exp
            {
	      $$ = g_strdup_printf ("not (%s)", $2);
	      g_free ($2);
	    }
        | '(' exp ')'
            {
	      $$ = g_strdup_printf ("(%s)", $2);
	      g_free ($2);
	    }
        | '(' exp
            {
	      $$ = g_strdup_printf ("(%s)", $2);
	      g_free ($2);
	    }

        /* Deal with things like not add by ignoring one of the terms.  */
        | exp error
            {
	      g_warning ("Parse error parsing `...%.*s...' (characters %d-%d)",
			 @2.last_column - @2.first_column + 1,
			 search_string + @2.first_column,
			 @2.first_column, @2.last_column);
	      yyerrok;

	      $$ = $1;
	    }
;
%%
int
yylex (YYSTYPE *lvalp, YYLTYPE *llocp, char *search_string, void **state)
{
  int pos = (int) *state;

  g_assert (pos <= strlen (search_string));

 retry:
  llocp->first_column = pos;
  llocp->last_column = pos;

  switch (search_string[pos])
    {
    case ' ':
    case '\t':
    case '\n':
      pos ++;
      goto retry;
    case '(':
    case ')':
      * (int *) state = pos + 1;
      return search_string[pos];
    case 0:
      /* We're done.  */
      return 0;
    default:
      break;
    }

  int len = strcspn (search_string + pos, " \t()");
  g_assert (len > 0);

  char *tok = g_malloc (len + 1);
  memcpy (tok, search_string + pos, len);
  tok[len] = 0;

  *lvalp = tok;

  llocp->first_column = pos;
  llocp->last_column = pos + len - 1;
  * (int *) state = pos + len;

  if (strcasecmp (tok, "and") == 0)
    {
      g_free (tok);
      return AND;
    }
  if (strcasecmp (tok, "or") == 0)
    {
      g_free (tok);
      return OR;
    }
  if (strcasecmp (tok, "not") == 0)
    {
      g_free (tok);
      return NOT;
    }
  return TERM;
}

void
yyerror (YYLTYPE *llocp, char *search_string, char **result, void **state,
	 char const *error)
{
  g_warning ("Parsing `%s': %s", search_string, error);
}
