/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib.h>
#include <glib/gmarkup.h>
#include <string.h>
#include <libintl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <gpe/errorbox.h>

#include "playlist_db.h"
#include "player.h"

#define _(x) gettext(x)

struct playlist_context {
  gchar *filename;
  struct playlist *playlist;
  struct playlist *cur_playlist;
  gchar **textptr;
  int first_element_seen;
};

void playlist_xml_start_element (GMarkupParseContext *context,
                          const gchar         *element_name,
                          const gchar        **attribute_names,
                          const gchar        **attribute_values,
                          gpointer             user_data,
                          GError             **error)
{
  struct playlist_context *pc = (struct playlist_context *)user_data;
  if (!pc->first_element_seen) {
    pc->first_element_seen = 1;
    if (strcmp(element_name, "playlist") != 0) {
      *error = g_error_new(g_quark_from_string("playlist-xml"),
			   -22, "Playlist description has wrong document type");
      return;
    }
  }
  if (strcmp(element_name, "list") == 0
      || strcmp(element_name, "playlist") == 0) {
    struct playlist *pl = playlist_new_list ();
    //fprintf(stderr, "newlist %s pl=%p\n", element_name, pl);
    if (!pc->playlist) {
      pc->playlist = pl;
      pl->title = g_path_get_basename(pc->filename);
    }
    pl->parent = pc->cur_playlist;
    pc->cur_playlist = pl;
    pc->textptr = NULL;
  } else if (strcmp(element_name, "title") == 0) {
    pc->textptr = &pc->cur_playlist->title;
  } else if (strcmp(element_name, "track") == 0) {
    struct playlist *pl = playlist_new_track ();
    //fprintf(stderr, "newltrack pl=%p\n", pl);
    pl->parent = pc->cur_playlist;
    pc->cur_playlist = pl;
    pc->textptr = NULL;
  } else if (strcmp(element_name, "url") == 0) {
    pc->textptr = &pc->cur_playlist->data.track.url;
  } else if (strcmp(element_name, "artist") == 0) {
    pc->textptr = &pc->cur_playlist->data.track.artist;
  } else if (strcmp(element_name, "album") == 0) {
    pc->textptr = &pc->cur_playlist->data.track.album;
  }
}

/* Called for close tags </foo> */
void playlist_xml_end_element (GMarkupParseContext *context,
			       const gchar         *element_name,
			       gpointer             user_data,
			       GError             **error)
{
  struct playlist_context *pc = (struct playlist_context *)user_data;
  if ((strcmp(element_name, "playlist") == 0)
      || (strcmp(element_name, "list") == 0)
      || (strcmp(element_name, "track") == 0)) {
    /* this playlist/track description done, append it to parent's list of playlists/tracks */
    struct playlist *cur = pc->cur_playlist;
    struct playlist *parent = cur->parent;
    //fprintf(stderr, "endlist %s pl=%p parent=%p pc->playlist=%p\n", element_name, cur, parent, pc->playlist);
    if (parent)
      parent->data.list = g_slist_append (parent->data.list, cur);

    if (cur->type == ITEM_TYPE_TRACK) {
      if (cur->title == NULL)
	cur->title = g_strdup(cur->data.track.url);
      player_fill_in_playlist (cur);
    }
    /* now pop stack */
    pc->cur_playlist = parent;
  } else if ((strcmp(element_name, "title") == 0)
	     || (strcmp(element_name, "url") == 0)
	     || (strcmp(element_name, "artist") == 0) 
	     || (strcmp(element_name, "album") == 0) 
	     ) {
    pc->textptr = NULL;
  }
}

  /* Called for character data */
  /* text is not nul-terminated */
void playlist_xml_text (GMarkupParseContext *context,
			const gchar         *text,
			gsize                text_len,  
			gpointer             user_data,
			GError             **error)
{
  struct playlist_context *pc = (struct playlist_context *)user_data;
  if (pc->textptr)
    *pc->textptr = g_strndup(text, text_len);
}

GMarkupParser parser = {
  .start_element = playlist_xml_start_element,
  .end_element = playlist_xml_end_element,
  .text = playlist_xml_text,
  .passthrough = NULL,
  .error = NULL
};

struct playlist *
playlist_xml_load (gchar *name)
{
  struct playlist_context playlist_context = { name, NULL, NULL, 0 };
  GMarkupParseContext *context = g_markup_parse_context_new (&parser, 0,
							     &playlist_context, NULL);
  int fd = open(name, O_RDONLY, 0);

  if (fd < 0)
    {
      gpe_perror_box (name);
      return FALSE;
    }

  while (1) {
    gchar text[1024];
    int textlen = read(fd, text, sizeof(text));
    GError *error;
    if (textlen <= 0) {
      break;
    }
    if (g_markup_parse_context_parse (context, text, textlen, &error) == FALSE) {
      gpe_error_box ("Error parsing xml playlist");
      g_markup_parse_context_free (context);
      return FALSE;
    }
  }

  if (playlist_context.playlist == NULL)
    {
      gpe_error_box ("Playlist description is empty");
      g_markup_parse_context_free (context);
      return FALSE;
    }

  g_markup_parse_context_free (context);
  if (playlist_context.playlist 
      && playlist_context.playlist->data.list
      && playlist_context.playlist->data.list->next == NULL) {
    /* top level playlist (for the file) contains singleton, unwrap and return */
    struct playlist *tmp = playlist_context.playlist;
    playlist_context.playlist = tmp->data.list->data;
    g_free(tmp);
  }
  return playlist_context.playlist;
}
