#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <id3tag.h>

#include "playlist_db.h"

/* Function stolen from mpg321 */

/* Convenience for retrieving already formatted id3 data
 * what parameter is one of
 *  ID3_FRAME_TITLE
 *  ID3_FRAME_ARTIST
 *  ID3_FRAME_ALBUM
 *  ID3_FRAME_YEAR
 *  ID3_FRAME_COMMENT
 *  ID3_FRAME_GENRE
 * It allocates a new string. Free it later.
 * NULL if no tag or error.
 */
static gchar *
playlist_id3_get_tag (struct id3_tag const *tag, char const *what, unsigned int maxlen)
{
  struct id3_frame const *frame;
  union id3_field const *field;
  int nstrings;
  int avail;
  int j;
  int tocopy;
  int len;
  char printable [1024];
  char *retval = NULL;
  id3_ucs4_t const *ucs4;
  id3_utf8_t *utf8;
  
  memset (printable, '\0', sizeof (printable));
  avail = sizeof (printable);
  
  if (strcmp (what, ID3_FRAME_COMMENT) == 0)
    {
      /*There may be sth wrong. I did not fully understand how to use
	libid3tag for retrieving comments  */
      j=0;
      frame = id3_tag_findframe(tag, ID3_FRAME_COMMENT, j++);
      if (!frame) 
	return NULL;
      ucs4 = id3_field_getfullstring (&frame->fields[3]);
      if (!ucs4) 
	return NULL;
      utf8 = id3_ucs4_latin1duplicate (ucs4);
      if (!utf8 || utf8[0] == 0) 
	return NULL;
      len = strlen (utf8);
      if (avail > len)
	tocopy = len;
      else
	tocopy = 0;
      if (!tocopy) 
	return (NULL);
      avail-=tocopy;
      strncat (printable, utf8, tocopy);
      free (utf8);
    }
  else
    {
      frame = id3_tag_findframe (tag, what, 0);
      if (!frame) 
	return NULL;
      field = &frame->fields[1];
      nstrings = id3_field_getnstrings(field);
      for (j=0; j<nstrings; ++j)
	{
	  ucs4 = id3_field_getstrings(field, j);
	  if (!ucs4) 
	    return NULL;
	  if (strcmp (what, ID3_FRAME_GENRE) == 0)
	    ucs4 = id3_genre_name (ucs4);
	  utf8 = id3_ucs4_utf8duplicate (ucs4);
	  if (!utf8) 
	    break;
	  len = strlen (utf8);
	  if (avail > len)
	    tocopy = len;
	  else
	    tocopy = 0;
	  if (!tocopy) 
	    break;
	  avail -= tocopy;
	  strncat (printable, utf8, tocopy);
	  free (utf8);
        }
    }

  retval = g_malloc (maxlen + 1);
  if (!retval) 
    return NULL;
  
  strncpy (retval, printable, maxlen);
  retval[maxlen] = '\0';
  
  len = strlen(printable);
  if (maxlen > len)
    memset (retval + len, ' ', maxlen - len);
  
  return retval;
}

gboolean
playlist_fill_id3_data (struct playlist *p)
{
  struct id3_file *id3struct;
  struct id3_tag *id3tag;
  gboolean rc = TRUE;

  id3struct = id3_file_open (p->data.track.url, ID3_FILE_MODE_READONLY);
  if (id3struct == NULL)
    return FALSE;

  id3tag = id3_file_tag (id3struct);

  if (id3tag)
    {
      p->title = playlist_id3_get_tag (id3tag, ID3_FRAME_TITLE, 30);
      p->data.track.artist = playlist_id3_get_tag (id3tag, ID3_FRAME_ARTIST, 30);
      p->data.track.album = playlist_id3_get_tag (id3tag, ID3_FRAME_ALBUM, 30);
    }
  else
    rc = FALSE;

  id3_file_close (id3struct);
  return rc;
}
