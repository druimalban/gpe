#include <config.h>

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <locale.h>
#include "desktop_file.h"


#include <libintl.h>
#define _(x) gettext ((x))
#define N_(x) x


typedef struct _GnomeDesktopFileSection GnomeDesktopFileSection;
typedef struct _GnomeDesktopFileLine GnomeDesktopFileLine;
typedef struct _GnomeDesktopFileParser GnomeDesktopFileParser;

struct _GnomeDesktopFileSection {
  GQuark section_name; /* 0 means just a comment block (before any section) */
  gint n_lines;
  GnomeDesktopFileLine *lines;
  gint n_allocated_lines;
};

struct _GnomeDesktopFileLine {
  GQuark key; /* 0 means comment or blank line in value */
  char *locale;
  gchar *value;
};

struct _GnomeDesktopFile {
  gint n_sections;
  GnomeDesktopFileSection *sections;
  gint n_allocated_sections;
  gint main_section;
  GnomeDesktopFileEncoding encoding;
};

struct _GnomeDesktopFileParser {
  GnomeDesktopFile *df;
  gint current_section;
  gint line_nr;
  char *line;
};

#define VALID_KEY_CHAR 1
#define VALID_LOCALE_CHAR 2
guchar valid[256] = { 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x3 , 0x2 , 0x0 , 
   0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x2 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 
   0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x0 , 0x0 , 0x0 , 0x0 , 0x2 , 
   0x0 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 
   0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
};

static void                     report_error   (GnomeDesktopFileParser   *parser,
						char                     *message,
						GnomeDesktopParseError    error_code,
						GError                  **error);
static GnomeDesktopFileSection *lookup_section (GnomeDesktopFile         *df,
						const char               *section);
static GnomeDesktopFileLine *   lookup_line    (GnomeDesktopFile         *df,
						GnomeDesktopFileSection  *section,
						const char               *keyname,
						const char               *locale);




GQuark
gnome_desktop_parse_error_quark (void)
{
  static GQuark quark;
  if (!quark)
    quark = g_quark_from_static_string ("g_desktop_parse_error");

  return quark;
}

static void
parser_free (GnomeDesktopFileParser *parser)
{
  gnome_desktop_file_free (parser->df);
}

static void
gnome_desktop_file_line_free (GnomeDesktopFileLine *line)
{
  g_free (line->locale);
  g_free (line->value);
}

static void
gnome_desktop_file_section_free (GnomeDesktopFileSection *section)
{
  int i;

  for (i = 0; i < section->n_lines; i++)
    gnome_desktop_file_line_free (&section->lines[i]);
  
  g_free (section->lines);
}

void
gnome_desktop_file_free (GnomeDesktopFile *df)
{
  int i;

  for (i = 0; i < df->n_sections; i++)
    gnome_desktop_file_section_free (&df->sections[i]);
  g_free (df->sections);

  g_free (df);
}

static void
grow_lines_in_section (GnomeDesktopFileSection *section)
{
  int new_n_lines;

  if (section->n_allocated_lines == 0)
    new_n_lines = 1;
  else
    new_n_lines = section->n_allocated_lines*2;

  section->lines = g_realloc (section->lines,
			      sizeof (GnomeDesktopFileLine) * new_n_lines);
  section->n_allocated_lines = new_n_lines;
}

static void
grow_sections (GnomeDesktopFile *df)
{
  int new_n_sections;

  if (df->n_allocated_sections == 0)
    new_n_sections = 1;
  else
    new_n_sections = df->n_allocated_sections*2;

  df->sections = g_realloc (df->sections,
                            sizeof (GnomeDesktopFileSection) * new_n_sections);
  df->n_allocated_sections = new_n_sections;
}

static gchar *
unescape_string (gchar *str, gint len)
{
  gchar *res;
  gchar *p, *q;
  gchar *end;

  /* len + 1 is enough, because unescaping never makes the
   * string longer */
  res = g_new (gchar, len + 1);
  p = str;
  q = res;
  end = str + len;

  while (p < end)
    {
      if (*p == 0)
	{
	  /* Found an embedded null */
	  g_free (res);
	  return NULL;
	}
      if (*p == '\\')
	{
	  p++;
	  if (p >= end)
	    {
	      /* Escape at end of string */
	      g_free (res);
	      return NULL;
	    }
	  
	  switch (*p)
	    {
	    case 's':
              *q++ = ' ';
              break;
           case 't':
              *q++ = '\t';
              break;
           case 'n':
              *q++ = '\n';
              break;
           case 'r':
              *q++ = '\r';
              break;
           case '\\':
              *q++ = '\\';
              break;
           default:
	     /* Invalid escape code */
	     g_free (res);
	     return NULL;
	    }
	  p++;
	}
      else
	*q++ = *p++;
    }
  *q = 0;

  return res;
}

static gchar *
escape_string (const gchar *str, gboolean escape_first_space)
{
  gchar *res;
  char *q;
  const gchar *p;
  const gchar *end;

  /* len + 1 is enough, because unescaping never makes the
   * string longer */
  res = g_new (gchar, strlen (str)*2 + 1);
  
  p = str;
  q = res;
  end = str + strlen (str);

  while (*p)
    {
      if (*p == ' ')
	{
	  if (escape_first_space && p == str)
	    {
	      *q++ = '\\';
	      *q++ = 's';
	    }
	  else
	    *q++ = ' ';
	}
      else if (*p == '\\')
	{
	  *q++ = '\\';
	  *q++ = '\\';
	}
      else if (*p == '\t')
	{
	  *q++ = '\\';
	  *q++ = 't';
	}
      else if (*p == '\n')
	{
	  *q++ = '\\';
	  *q++ = 'n';
	}
      else if (*p == '\r')
	{
	  *q++ = '\\';
	  *q++ = 'r';
	}
      else
	*q++ = *p;
      p++;
    }
  *q = 0;

  return res;
}


static GnomeDesktopFileSection* 
new_section (GnomeDesktopFile       *df,
             const char             *name,
             GError                **error)
{
  int n;
  gboolean is_main = FALSE;

  if (name &&
      (strcmp (name, "Desktop Entry") == 0 ||
       strcmp (name, "KDE Desktop Entry") == 0))
    is_main = TRUE;

  if (is_main &&
      df->main_section >= 0)
    {
      g_set_error (error,
                   GNOME_DESKTOP_PARSE_ERROR,
                   GNOME_DESKTOP_PARSE_ERROR_INVALID_SYNTAX,                   
                   "Two [Desktop Entry] or [KDE Desktop Entry] sections seen");

      return NULL;
    }
  
  if (df->n_allocated_sections == df->n_sections)
    grow_sections (df);

  if (df->n_sections == 1 &&
      df->sections[0].section_name == 0 &&
      df->sections[0].n_lines == 0)
    {
      if (!name)
	g_warning ("non-initial NULL section\n");
      
      /* The initial section was empty. Piggyback on it. */
      df->sections[0].section_name = g_quark_from_string (name);

      if (is_main)
	df->main_section = 0;
      
      return &df->sections[0];
    }
  
  n = df->n_sections++;

  if (is_main)
    df->main_section = n;
  
  if (name)
    df->sections[n].section_name = g_quark_from_string (name);
  else
    df->sections[n].section_name = 0;

  df->sections[n].n_lines = 0;
  df->sections[n].lines = NULL;
  df->sections[n].n_allocated_lines = 0;

  grow_lines_in_section (&df->sections[n]);

  return &df->sections[n];
}

static GnomeDesktopFileSection* 
open_section (GnomeDesktopFileParser *parser,
              char                   *name,
              GError                **error)
{  
  GnomeDesktopFileSection *section;

  section = new_section (parser->df, name, error);
  if (section == NULL)
    return NULL;
  
  parser->current_section = parser->df->n_sections - 1;
  g_assert (&parser->df->sections[parser->current_section] == section);
  
  return section;
}

static GnomeDesktopFileLine*
new_line_in_section (GnomeDesktopFileSection *section)
{
  GnomeDesktopFileLine *line;
  
  if (section->n_allocated_lines == section->n_lines)
    grow_lines_in_section (section);

  line = &section->lines[section->n_lines++];

  memset (line, 0, sizeof (GnomeDesktopFileLine));
  
  return line;
}

static GnomeDesktopFileLine *
new_line (GnomeDesktopFileParser *parser)
{
  GnomeDesktopFileSection *section;

  section = &parser->df->sections[parser->current_section];

  return new_line_in_section (section);
}

static gboolean
is_blank_line (GnomeDesktopFileParser *parser)
{
  gchar *p;

  p = parser->line;

  while (*p && *p != '\n')
    {
      if (!g_ascii_isspace (*p))
	return FALSE;

      p++;
    }
  return TRUE;
}

static void
parse_comment_or_blank (GnomeDesktopFileParser *parser)
{
  GnomeDesktopFileLine *line;
  gchar *line_end;

  line_end = strchr (parser->line, '\n');
  if (line_end == NULL)
    line_end = parser->line + strlen (parser->line);

  line = new_line (parser);
  
  line->value = g_strndup (parser->line, line_end - parser->line);

    if (*line_end == '\n')
    ++line_end;
  else if (*line_end == '\0')
    line_end = NULL;
  
  parser->line = line_end;

  parser->line_nr++;
}

static gboolean
is_valid_section_name (const char *name)
{
  /* 5. Group names may contain all ASCII characters except for control characters and '[' and ']'. */
  
  while (*name)
    {
      if (!(g_ascii_isprint (*name) || *name == '\n' || *name == '\t'))
	return FALSE;
      
      name++;
    }
  
  return TRUE;
}

static gboolean
parse_section_start (GnomeDesktopFileParser *parser, GError **error)
{
  gchar *line_end;
  gchar *section_name;

  line_end = strchr (parser->line, '\n');
  if (line_end == NULL)
    line_end = parser->line + strlen (parser->line);

  if (line_end - parser->line <= 2 ||
      line_end[-1] != ']')
    {
      report_error (parser, "Invalid syntax for section header", GNOME_DESKTOP_PARSE_ERROR_INVALID_SYNTAX, error);
      parser_free (parser);
      return FALSE;
    }

  section_name = unescape_string (parser->line + 1, line_end - parser->line - 2);

  if (section_name == NULL)
    {
      report_error (parser, "Invalid escaping in section name", GNOME_DESKTOP_PARSE_ERROR_INVALID_ESCAPES, error);
      parser_free (parser);
      return FALSE;
    }

  if (!is_valid_section_name (section_name))
    {
      report_error (parser, "Invalid characters in section name", GNOME_DESKTOP_PARSE_ERROR_INVALID_CHARS, error);
      parser_free (parser);
      g_free (section_name);
      return FALSE;
    }
  
  if (open_section (parser, section_name, error) == NULL)
    {
      g_free (section_name);
      return FALSE;
    }

  if (*line_end == '\n')
    ++line_end;
  else if (*line_end == '\0')
    line_end = NULL;
  
  parser->line = line_end;
  
  parser->line_nr++;

  g_free (section_name);
  
  return TRUE;
}

static gboolean
parse_key_value (GnomeDesktopFileParser *parser, GError **error)
{
  GnomeDesktopFileLine *line;
  gchar *line_end;
  gchar *key_start;
  gchar *key_end;
  gchar *locale_start = NULL;
  gchar *locale_end = NULL;
  gchar *value_start;
  gchar *value;
  gchar *p;

  line_end = strchr (parser->line, '\n');
  if (line_end == NULL)
    line_end = parser->line + strlen (parser->line);

  p = parser->line;
  key_start = p;
  while (p < line_end &&
	 (valid[(guchar)*p] & VALID_KEY_CHAR)) 
    p++;
  key_end = p;

  if (key_start == key_end)
    {
      report_error (parser, "Empty key name", GNOME_DESKTOP_PARSE_ERROR_INVALID_SYNTAX, error);
      parser_free (parser);
      return FALSE;
    }

  if (p < line_end && *p == '[')
    {
      p++;
      locale_start = p;
      while (p < line_end &&
	     (valid[(guchar)*p] & VALID_LOCALE_CHAR)) 
	p++;
      locale_end = p;

      if (p == line_end)
	{
	  report_error (parser, "Unterminated locale specification in key", GNOME_DESKTOP_PARSE_ERROR_INVALID_SYNTAX, error);
	  parser_free (parser);
	  return FALSE;
	}
      
      if (*p != ']')
	{
	  report_error (parser, "Invalid characters in locale name", GNOME_DESKTOP_PARSE_ERROR_INVALID_CHARS, error);
	  parser_free (parser);
	  return FALSE;
	}
      
      if (locale_start == locale_end)
	{
	  report_error (parser, "Empty locale name", GNOME_DESKTOP_PARSE_ERROR_INVALID_SYNTAX, error);
	  parser_free (parser);
	  return FALSE;
	}
      p++;
    }
  
  /* Skip space before '=' */
  while (p < line_end && *p == ' ')
    p++;

  if (p < line_end && *p != '=')
    {
      report_error (parser, "Invalid characters in key name", GNOME_DESKTOP_PARSE_ERROR_INVALID_CHARS, error);
      parser_free (parser);
      return FALSE;
    }

  if (p == line_end)
    {
      report_error (parser, "No '=' in key/value pair", GNOME_DESKTOP_PARSE_ERROR_INVALID_SYNTAX, error);
      parser_free (parser);
      return FALSE;
    }

  /* Skip the '=' */
  p++;

  /* Skip space after '=' */
  while (p < line_end && *p == ' ')
    p++;

  value_start = p;

  value = unescape_string (value_start, line_end - value_start);
  if (value == NULL)
    {
      report_error (parser, "Invalid escaping in value", GNOME_DESKTOP_PARSE_ERROR_INVALID_ESCAPES, error);
      parser_free (parser);
      return FALSE;
    }

  line = new_line (parser);
  line->key = g_quark_from_static_string (g_strndup (key_start, key_end - key_start));
  if (locale_start)
    line->locale = g_strndup (locale_start, locale_end - locale_start);
  line->value = value;

  if (*line_end == '\n')
    ++line_end;
  else if (*line_end == '\0')
    line_end = NULL;
  
  parser->line = line_end;
  parser->line_nr++;
  
  return TRUE;
}


static void
report_error (GnomeDesktopFileParser *parser,
	      char                   *message,
	      GnomeDesktopParseError  error_code,
	      GError                **error)
{
  GnomeDesktopFileSection *section;
  const gchar *section_name = NULL;

  section = &parser->df->sections[parser->current_section];

  if (section->section_name)
    section_name = g_quark_to_string (section->section_name);
  
  if (error)
    {
      if (section_name)
	*error = g_error_new (GNOME_DESKTOP_PARSE_ERROR,
			      error_code,
			      "Error in section %s at line %d: %s", section_name, parser->line_nr, message);
      else
	*error = g_error_new (GNOME_DESKTOP_PARSE_ERROR,
			      error_code,
			      "Error at line %d: %s", parser->line_nr, message);
    }
}


GnomeDesktopFile *
gnome_desktop_file_new_from_string (char                       *data,
				    GError                    **error)
{
  GnomeDesktopFileParser parser;
  GnomeDesktopFileLine *line;

  parser.df = g_new0 (GnomeDesktopFile, 1);
  parser.df->main_section = -1;
  parser.current_section = -1;

  parser.line_nr = 1;

  parser.line = data;

  /* Put any initial comments in a NULL segment */
  open_section (&parser, NULL, NULL);
  
  while (parser.line && *parser.line)
    {
      if (*parser.line == '[') {
	if (!parse_section_start (&parser, error))
	  return NULL;
      } else if (is_blank_line (&parser) ||
		 *parser.line == '#')
	parse_comment_or_blank (&parser);
      else
	{
	  if (!parse_key_value (&parser, error))
	    return NULL;
	}
    }

  if (parser.df->main_section >= 0)
    {
      line = lookup_line (parser.df,
			  &parser.df->sections[parser.df->main_section],
			  "Encoding", NULL);
      if (line)
	{
	  if (strcmp (line->value, "UTF-8") == 0)
	    parser.df->encoding = GNOME_DESKTOP_FILE_ENCODING_UTF8;
	  else if (strcmp (line->value, "Legacy-Mixed") == 0)
	    parser.df->encoding = GNOME_DESKTOP_FILE_ENCODING_LEGACY;
	  else
	    parser.df->encoding = GNOME_DESKTOP_FILE_ENCODING_UNKNOWN;
	}
      else
	{
	  /* No encoding specified. We have to guess
	   * If the whole file validates as UTF-8 it's probably UTF-8.
	   * Otherwise we guess it's a Legacy-Mixed
	   */
	  if (g_utf8_validate (data, -1, NULL))
	    parser.df->encoding = GNOME_DESKTOP_FILE_ENCODING_UTF8;
	  else
	    parser.df->encoding = GNOME_DESKTOP_FILE_ENCODING_LEGACY;
	}
	
    }
  else
    parser.df->encoding = GNOME_DESKTOP_FILE_ENCODING_UNKNOWN;
  
  return parser.df;
}

GnomeDesktopFileEncoding
gnome_desktop_file_get_encoding (GnomeDesktopFile *df)
{
  return df->encoding;
}

char *
gnome_desktop_file_to_string (GnomeDesktopFile *df)
{
  GnomeDesktopFileSection *section;
  GnomeDesktopFileLine *line;
  GString *str;
  char *s;
  int i, j;
  
  str = g_string_sized_new (800);

  for (i = 0; i < df->n_sections; i ++)
    {
      section = &df->sections[i];

      if (section->section_name)
	{
	  g_string_append_c (str, '[');
	  s = escape_string (g_quark_to_string (section->section_name), FALSE);
	  g_string_append (str, s);
	  g_free (s);
	  g_string_append (str, "]\n");
	}
      
      for (j = 0; j < section->n_lines; j++)
	{
	  line = &section->lines[j];
	  
	  if (line->key == 0)
	    {
	      g_string_append (str, line->value);
	      g_string_append_c (str, '\n');
	    }
	  else
	    {
	      g_string_append (str, g_quark_to_string (line->key));
	      if (line->locale)
		{
		  g_string_append_c (str, '[');
		  g_string_append (str, line->locale);
		  g_string_append_c (str, ']');
		}
	      g_string_append_c (str, '=');
	      s = escape_string (line->value, TRUE);
	      g_string_append (str, s);
	      g_free (s);
	      g_string_append_c (str, '\n');
	    }
	}
    }
  
  return g_string_free (str, FALSE);
}

static GnomeDesktopFileSection *
lookup_section (GnomeDesktopFile  *df,
		const char        *section_name)
{
  GnomeDesktopFileSection *section;
  GQuark section_quark;
  int i;
  
  if (section_name == NULL)
    {
      if (df->main_section < 0)
        return NULL;
      else
        return &df->sections[df->main_section];
    }
  
  section_quark = g_quark_try_string (section_name);
  if (section_quark == 0)
    return NULL;
  
  for (i = 0; i < df->n_sections; i ++)
    {
      section = &df->sections[i];

      if (section->section_name == section_quark)
	return section;
    }
  return NULL;
}

static GnomeDesktopFileLine *
lookup_line (GnomeDesktopFile        *df,
	     GnomeDesktopFileSection *section,
	     const char              *keyname,
	     const char              *locale)
{
  GnomeDesktopFileLine *line;
  GQuark key_quark;
  int i;

  key_quark = g_quark_try_string (keyname);
  if (key_quark == 0)
    return NULL;
  
  for (i = 0; i < section->n_lines; i++)
    {
      line = &section->lines[i];
      
      if (line->key == key_quark &&
	  ((locale == NULL && line->locale == NULL) ||
	   (locale != NULL && line->locale != NULL && strcmp (locale, line->locale) == 0)))
	return line;
    }
  
  return NULL;
}

gboolean
gnome_desktop_file_get_raw (GnomeDesktopFile  *df,
			    const char        *section_name,
			    const char        *keyname,
			    const char        *locale,
			    const char       **val)
{
  GnomeDesktopFileSection *section;
  GnomeDesktopFileLine *line;

  *val = NULL;

  section = lookup_section (df, section_name);
  if (!section)
    return FALSE;

  line = lookup_line (df,
		      section,
		      keyname,
		      locale);

  if (!line)
    return FALSE;
  
  *val = line->value;
  
  return TRUE;
}

void
gnome_desktop_file_foreach_section (GnomeDesktopFile            *df,
				    GnomeDesktopFileSectionFunc  func,
				    gpointer                     user_data)
{
  GnomeDesktopFileSection *section;
  int i;

  for (i = 0; i < df->n_sections; i ++)
    {
      section = &df->sections[i];

      (*func) (df, g_quark_to_string (section->section_name),  user_data);
    }
  return;
}

void
gnome_desktop_file_foreach_key (GnomeDesktopFile            *df,
				const char                  *section_name,
				gboolean                     include_localized,
				GnomeDesktopFileLineFunc     func,
				gpointer                     user_data)
{
  GnomeDesktopFileSection *section;
  GnomeDesktopFileLine *line;
  int i;
  
  section = lookup_section (df, section_name);
  if (!section)
    return;
  
  for (i = 0; i < section->n_lines; i++)
    {
      line = &section->lines[i];

      (*func) (df, g_quark_to_string (line->key), line->locale, line->value, user_data);
    }
  
  return;
}

void
gnome_desktop_file_rename_section  (GnomeDesktopFile            *df,
                                    const char                  *old_name,
                                    const char                  *new_name)
{
  GnomeDesktopFileSection *section;  

  g_return_if_fail (new_name != NULL);
  
  if (old_name == NULL &&
      df->main_section < 0)
    old_name = "Desktop Entry";

  section = lookup_section (df, old_name);
  if (section == NULL)
    return;

  section->section_name = g_quark_from_string (new_name);
}

gboolean
gnome_desktop_file_has_section (GnomeDesktopFile *df,
                                const char       *name)
{
  GnomeDesktopFileSection *section;  

  g_return_val_if_fail (name != NULL, FALSE);  

  section = lookup_section (df, name);

  return section != NULL;
}

GnomeDesktopFile*
gnome_desktop_file_load (const char    *filename,
                         GError       **error)
{
  char *contents;
  GnomeDesktopFile *df;
  
  if (!g_file_get_contents (filename, &contents,
			    NULL, error))
    return NULL;

  df = gnome_desktop_file_new_from_string (contents, error);

  g_free (contents);

  return df;
}


gboolean
gnome_desktop_file_save (GnomeDesktopFile *df,
                         const char       *path,
                         int               mode,
                         GError          **error)
{
  char *str;
  FILE *f;
  int fd;
  char *tmp_filename;

  tmp_filename = g_strconcat (path, ".new-tmp", NULL);
  
  f = fopen (tmp_filename, "w");
  if (f == NULL)
    {
      g_set_error (error, 
                   G_FILE_ERROR,
                   g_file_error_from_errno (errno),
                   _("Failed to open \"%s\": %s"),
                   tmp_filename, g_strerror (errno));
      g_free (tmp_filename);
      return FALSE;
    }
  
  fd = fileno (f);
  
  if (fchmod (fd, mode) < 0)
    {
      g_set_error (error, G_FILE_ERROR,
                   g_file_error_from_errno (errno),
                   _("Failed to set permissions %o on \"%s\": %s"),
                   mode, tmp_filename, g_strerror (errno));

      fclose (f);
      unlink (tmp_filename);
      g_free (tmp_filename);
      
      return FALSE;
    }

  str = gnome_desktop_file_to_string (df);
  
  if (fputs (str, f) < 0)
    {
      g_set_error (error,
                   G_FILE_ERROR,
                   g_file_error_from_errno (errno),
                   _("Failed to write to \"%s\": %s"),
                   tmp_filename, g_strerror (errno));

      fclose (f);
      unlink (tmp_filename);
      g_free (str);
      g_free (tmp_filename);
      
      return FALSE;
    }

  g_free (str);
  
  if (fclose (f) < 0)
    {
      g_set_error (error,
                   G_FILE_ERROR,
                   g_file_error_from_errno (errno),
                   _("Failed to close \"%s\": %s"),
                   tmp_filename, g_strerror (errno));

      unlink (tmp_filename);
      g_free (tmp_filename);
      
      return FALSE;
    }

  if (rename (tmp_filename, path) < 0)
    {
      g_set_error (error,
                   G_FILE_ERROR,
                   g_file_error_from_errno (errno),
                   _("Failed to rename \"%s\" to \"%s\": %s"),
                   tmp_filename, path, g_strerror (errno));
      unlink (tmp_filename);
      g_free (tmp_filename);

      return FALSE;
    }

  g_free (tmp_filename);
  
  return TRUE;
}

/* Mask for components of locale spec. The ordering here is from
 * least significant to most significant
 */
enum
{
  COMPONENT_CODESET =   1 << 0,
  COMPONENT_TERRITORY = 1 << 1,
  COMPONENT_MODIFIER =  1 << 2
};

/* Break an X/Open style locale specification into components
 */
static guint
explode_locale (const gchar *locale,
		gchar      **language, 
		gchar      **territory, 
		gchar      **codeset, 
		gchar      **modifier)
{
  const gchar *uscore_pos;
  const gchar *at_pos;
  const gchar *dot_pos;

  guint mask = 0;

  uscore_pos = strchr (locale, '_');
  dot_pos = strchr (uscore_pos ? uscore_pos : locale, '.');
  at_pos = strchr (dot_pos ? dot_pos : (uscore_pos ? uscore_pos : locale), '@');

  if (at_pos)
    {
      mask |= COMPONENT_MODIFIER;
      *modifier = g_strdup (at_pos);
    }
  else
    at_pos = locale + strlen (locale);

  if (dot_pos)
    {
      mask |= COMPONENT_CODESET;
      *codeset = g_new (gchar, 1 + at_pos - dot_pos);
      strncpy (*codeset, dot_pos, at_pos - dot_pos);
      (*codeset)[at_pos - dot_pos] = '\0';
    }
  else
    dot_pos = at_pos;

  if (uscore_pos)
    {
      mask |= COMPONENT_TERRITORY;
      *territory = g_new (gchar, 1 + dot_pos - uscore_pos);
      strncpy (*territory, uscore_pos, dot_pos - uscore_pos);
      (*territory)[dot_pos - uscore_pos] = '\0';
    }
  else
    uscore_pos = dot_pos;

  *language = g_new (gchar, 1 + uscore_pos - locale);
  strncpy (*language, locale, uscore_pos - locale);
  (*language)[uscore_pos - locale] = '\0';

  return mask;
}

gboolean
gnome_desktop_file_get_locale_string (GnomeDesktopFile  *df,
                                      const char        *section,
                                      const char        *keyname,
                                      char             **val)
{
  const char *raw;
  char *lang, *territory, *codeset, *modifier;
  const char *locale;
  char *with_territory;
  char *used_locale;
  GError *error;
  
  *val = NULL;
  
  lang = NULL;
  territory = NULL;
  codeset = NULL;
  modifier = NULL;
  used_locale = NULL;
  
  locale = setlocale (LC_MESSAGES, NULL);
  if (locale != NULL)
    explode_locale (locale, &lang, &territory, &codeset, &modifier);

  if (territory)
    with_territory = g_strconcat (lang, "_", territory, NULL);
  else
    with_territory = NULL;

  if (with_territory == NULL ||
      !gnome_desktop_file_get_raw (df, section, keyname, with_territory, &raw))
    {
      if (lang == NULL ||
          !gnome_desktop_file_get_raw (df, section, keyname, lang, &raw))
        {
          gnome_desktop_file_get_raw (df, section, keyname, NULL, &raw);
        }
      else
        {
          used_locale = lang;
          lang = NULL;
        }
    }
  else
    {
      used_locale = with_territory;
      with_territory = NULL;
    }

  g_free (lang);
  g_free (territory);
  g_free (codeset);
  g_free (modifier);
  g_free (with_territory);

  if (raw == NULL)
    return FALSE;
  
  if (gnome_desktop_file_get_encoding (df) == GNOME_DESKTOP_FILE_ENCODING_UTF8)
    {
      g_free (used_locale);
      *val = g_strdup (raw);
      return TRUE;      
    }
  else if (gnome_desktop_file_get_encoding (df) == GNOME_DESKTOP_FILE_ENCODING_LEGACY)
    {
      if (used_locale)
	{
          const char *encoding;
          
	  encoding = desktop_file_get_encoding_for_locale (used_locale);
          g_free (used_locale);
          
	  if (encoding)
	    {
              char *res;
              
	      error = NULL;
	      res = g_convert (raw, -1,            
			       "UTF-8",
			       encoding,
			       NULL,     
			       NULL,  
			       &error);

              if (res == NULL)
                {
                  g_printerr ("Error converting from UTF-8 to %s for key %s: %s\n",
                              encoding, keyname, error->message);
                  g_error_free (error);
                }

              *val = res;

              return *val != NULL;
	    }
	  else
            {
              g_printerr ("Don't know encoding for desktop file field %s with locale \"%s\"\n",
                          keyname, used_locale);
              g_free (used_locale);
              return FALSE;
            }
        }
      else
        {
          /* this is just ASCII, hopefully OK, though it's really a bit
           * broken to return it
           */
          *val = g_strdup (raw);
          return TRUE;
        }
    }
  else
    {
      g_printerr ("Desktop file doesn't have its encoding marked, can't parse it.\n");
      return FALSE;
    }
}

gboolean
gnome_desktop_file_get_string (GnomeDesktopFile   *df,
                               const char         *section,
                               const char         *keyname,
                               char              **val)
{
  const char *raw;
  
  *val = NULL;
  
  if (!gnome_desktop_file_get_raw (df, section, keyname, NULL, &raw))
    return FALSE;

  *val = g_strdup (raw);
  
  return TRUE;
}

gboolean
gnome_desktop_file_get_strings (GnomeDesktopFile   *df,
                                const char         *section,
                                const char         *keyname,
                                const char         *locale,
                                char             ***vals,
                                int                *len)
{
  const char *raw;
  char **retval;
  int i;
  
  if (vals)
    *vals = NULL;
  if (len)
    *len = 0;
  
  if (!gnome_desktop_file_get_raw (df, section, keyname, locale, &raw))
    return FALSE;

  retval = g_strsplit (raw, ";", G_MAXINT);

  i = 0;
  while (retval[i])
    ++i;

  /* Drop the empty string g_strsplit leaves in the vector since
   * our list of strings ends in ";"
   */
  --i;
  g_free (retval[i]);
  retval[i] = NULL;
  
  if (vals)
    *vals = retval;
  else
    g_strfreev (retval);

  if (len)
    *len = i;

  return TRUE;
}

void
gnome_desktop_file_set_raw (GnomeDesktopFile  *df,
                            const char        *section_name,
                            const char        *keyname,
                            const char        *locale,
                            const char        *value)
{
  GnomeDesktopFileSection *section;
  GnomeDesktopFileLine *line;
  

  if (section_name == NULL &&
      df->main_section < 0)
    section_name = "Desktop Entry";

  section = lookup_section (df, section_name);
  if (section == NULL)
    {
      section = new_section (df, section_name, NULL);
      g_assert (section);
    }

  line = lookup_line (df,
		      section,
		      keyname,
		      locale);

  if (line == NULL)
    line = new_line_in_section (section);

  line->key = g_quark_from_string (keyname);
  g_free (line->value);
  g_free (line->locale);
  line->value = g_strdup (value);
  line->locale = g_strdup (locale);
}

void
gnome_desktop_file_set_strings (GnomeDesktopFile  *df,
                                const char        *section_name,
                                const char        *keyname,
                                const char        *locale,
                                const char       **value)
{
  char *str;
  char *tmp;  
  
  tmp = g_strjoinv (";", (char**)value);
  str = g_strconcat (tmp, ";", NULL);
  g_free (tmp);
  gnome_desktop_file_set_raw (df, section_name, keyname, locale, str);
  g_free (str);
}


void
gnome_desktop_file_merge_string_into_list (GnomeDesktopFile *df,
                                           const char       *section,
                                           const char       *keyname,
                                           const char       *locale,
                                           const char       *value)
{
  char **values;
  int n_values;
  const char *raw;
  
  if (gnome_desktop_file_get_strings (df, section, keyname, locale,
                                      &values, &n_values))
    {
      /* Look for a duplicate */
      int i;
      gboolean found;

      found = FALSE;
      
      i = 0;
      while (i < n_values)
        {
          if (strcmp (values[i], value) == 0)
            {
              found = TRUE;
              break;
            }

          ++i;
        }

      g_strfreev (values);
      
      if (found)
        return; /* nothing to do */
    }

  gnome_desktop_file_get_raw (df, section, keyname, locale, &raw);
  
  {
    /* Append to current list */
    char *str;
    str = g_strconcat (raw ? raw : "", value, ";", NULL);
    gnome_desktop_file_set_raw (df, section, keyname, locale, str);
    g_free (str);
  }
}

void
gnome_desktop_file_remove_string_from_list (GnomeDesktopFile *df,
                                            const char        *section,
                                            const char        *keyname,
                                            const char        *locale,
                                            const char        *value)
{
  char **values;
  int n_values;
  
  if (gnome_desktop_file_get_strings (df, section, keyname, locale,
                                      &values, &n_values))
    {
      int i;
      int n_found;

      n_found = 0;
      
      i = 0;
      while (i < n_values)
        {
          if (n_found > 0)
            {
              g_free (values[i]);
              values[i] = values[i+1];
              values[i+1] = NULL;
            }

          if (((i+1) < n_values) &&
              strcmp (values[i], value) == 0)
            {
              ++n_found;
            }
          else
            {
              ++i;
            }
        }

      if (n_found > 0)
        gnome_desktop_file_set_strings (df, section, keyname, locale,
                                        (const char**) values);
      
      g_strfreev (values);
    }
}

#define N_LANG 30

/* #define VERIFY_CANONICAL_ENCODING_NAME */

struct {
  const char *encoding;
  const char *langs[N_LANG];
} known_encodings[] = {
  {"ARMSCII-8", {"by"}},
  {"BIG5", {"zh_TW"}},
  {"CP1251", {"be", "bg"}},
  {"EUC-CN", {"zh_TW"}},
  {"EUC-JP", {"ja"}},
  {"EUC-KR", {"ko"}},
  {"GEORGIAN-ACADEMY", {}},
  {"GEORGIAN-PS", {"ka"}},
  {"ISO-8859-1", {"br", "ca", "da", "de", "en", "es", "eu", "fi", "fr", "gl", "it", "nl", "wa", "no", "pt", "pt", "sv"}},
  {"ISO-8859-2", {"cs", "hr", "hu", "pl", "ro", "sk", "sl", "sq", "sr"}},
  {"ISO-8859-3", {"eo"}},
  {"ISO-8859-5", {"mk", "sp"}},
  {"ISO-8859-7", {"el"}},
  {"ISO-8859-9", {"tr"}},
  {"ISO-8859-13", {"lv", "lt", "mi"}},
  {"ISO-8859-14", {"ga", "cy"}},
  {"ISO-8859-15", {"et"}},
  {"KOI8-R", {"ru"}},
  {"KOI8-U", {"uk"}},
  {"TCVN-5712", {"vi"}},
  {"TIS-620", {"th"}},
  {"VISCII", {}},
};

struct {
  const char *alias;
  const char *value;
} enc_aliases[] = {
  {"GB2312", "EUC-CN"},
  {"TCVN", "TCVN-5712" }
};

static gboolean
aliases_equal (const char *enc1, const char *enc2)
{
  while (*enc1 && *enc2)
    {
      while (*enc1 == '-' ||
	     *enc1 == '.' ||
	     *enc1 == '_')
	enc1++;
      
      while (*enc2 == '-' ||
	     *enc2 == '.' ||
	     *enc2 == '_')
	enc2++;

      if (g_ascii_tolower (*enc1) != g_ascii_tolower (*enc2))
	return FALSE;
      enc1++;
      enc2++;
    }

  while (*enc1 == '-' ||
	 *enc1 == '.' ||
	 *enc1 == '_')
    enc1++;
  
  while (*enc2 == '-' ||
	 *enc2 == '.' ||
	 *enc2 == '_')
    enc2++;

  if (*enc1 || *enc2)
    return FALSE;
  
  return TRUE;
}

static const char *
get_canonical_encoding (const char *encoding)
{
  int i;
  for (i = 0; i < G_N_ELEMENTS (enc_aliases); i++)
    {
      if (aliases_equal (enc_aliases[i].alias, encoding))
	return enc_aliases[i].value;
    }

  for (i = 0; i < G_N_ELEMENTS (known_encodings); i++)
    {
      if (aliases_equal (known_encodings[i].encoding, encoding))
	return known_encodings[i].encoding;
    }
  
  return encoding;
}

static gboolean
lang_tag_matches (const char *l, const char *spec)
{
  char *l2;
  
  if (strcmp (l, spec) == 0)
    return TRUE;

  l2 = strchr (l, '_');

  if (l2 && strchr (spec, '_') == NULL &&
      strncmp (l, spec, l2 - l) == 0)
    return TRUE;
  
  return FALSE;
}

static const char *
get_encoding_from_lang (const char *lang)
{
  int i, j;
  
  for (i = 0; i < G_N_ELEMENTS (known_encodings); i++)
    {
      for (j = 0; j < N_LANG; j++)
	{
	  if (known_encodings[i].langs[j] && lang_tag_matches (lang, known_encodings[i].langs[j]))
	    return known_encodings[i].encoding;
	}
    }
  return NULL;
}

const char*
desktop_file_get_encoding_for_locale (const char *locale)
{
  char *encoding;

  encoding = strchr(locale, '.');

  if (encoding)
    {
      encoding++;

      return get_canonical_encoding (encoding);
    }
  
  return get_encoding_from_lang (locale);
}

static void
gnome_desktop_file_unset_internal (GnomeDesktopFile *df,
                                   const char       *section_name,
                                   const char       *keyname,
                                   const char       *locale,
                                   gboolean          all_locales)
{
  GnomeDesktopFileLine *line;
  GnomeDesktopFileSection *section;
  GQuark key_quark;
  int i;

  section = lookup_section (df, section_name);
  if (section == NULL)
    return;
  
  key_quark = g_quark_try_string (keyname);
  if (key_quark == 0)
    return;
  
  i = 0;
  while (i < section->n_lines)
    {
      line = &section->lines[i];
      
      if (line->key == key_quark &&
          (all_locales ||
           ((locale == NULL && line->locale == NULL) ||
            (locale != NULL && line->locale != NULL && strcmp (locale, line->locale) == 0))))
        {
          g_free (line->locale);
          g_free (line->value);

          if ((i+1) < section->n_lines)
            g_memmove (&section->lines[i], &section->lines[i+1],
                       (section->n_lines - i - 1) * sizeof (section->lines[0]));

          section->n_lines -= 1;
        }
      else
        {
          ++i;
        }
    }
}

void
gnome_desktop_file_unset (GnomeDesktopFile *df,
                          const char       *section,
                          const char       *keyname)
{
  gnome_desktop_file_unset_internal (df, section, keyname, NULL, TRUE);
}

void
gnome_desktop_file_unset_for_locale (GnomeDesktopFile *df,
                                     const char       *section,
                                     const char       *keyname,
                                     const char       *locale)
{
  gnome_desktop_file_unset_internal (df, section, keyname, locale, FALSE);
}

void
gnome_desktop_file_copy_key (GnomeDesktopFile *df,
                             const char       *section_name,
                             const char       *source_key,
                             const char       *dest_key)
{
  GnomeDesktopFileLine *line;
  GnomeDesktopFileSection *section;
  GQuark key_quark;
  int i;

  g_return_if_fail (source_key != NULL);
  g_return_if_fail (dest_key != NULL);
  g_return_if_fail (strcmp (source_key, dest_key) != 0);
  
  gnome_desktop_file_unset (df, section_name, dest_key);

  section = lookup_section (df, section_name);
  if (section == NULL)
    return;
  
  key_quark = g_quark_try_string (source_key);
  if (key_quark == 0)
    return;

  i = 0;
  while (i < section->n_lines)
    {
      line = &section->lines[i];
      
      if (line->key == key_quark)
        {
          /* Mmmm, modify the array we're iterating over...
           * should be safe as it just appends.
           */
          
          gnome_desktop_file_set_raw (df, section_name, dest_key,
                                      line->locale, line->value); 
        }

      ++i;
    }
}
