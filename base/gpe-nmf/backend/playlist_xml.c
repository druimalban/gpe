/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib.h>
#include <libxml/parser.h>

#include <gpe/errorbox.h>

#include "playlist_db.h"

static struct playlist *
playlist_parse_xml_track (xmlDocPtr doc, xmlNodePtr cur)
{
  struct playlist *i = playlist_new_track ();

  while (cur)
    {
      if (!xmlStrcmp (cur->name, "title"))
	i->title = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
      else if (!xmlStrcmp (cur->name, "url"))
	i->data.track.url = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
      else if (!xmlStrcmp (cur->name, "artist"))
	i->data.track.artist = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
      else if (!xmlStrcmp (cur->name, "album"))
	i->data.track.album = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);

      cur = cur->next;
    }

  decoder_fill_in_playlist (i);

  if (i->title == NULL)
    i->title = i->data.track.url;

  return i;
}

static struct playlist *
playlist_parse_xml (xmlDocPtr doc, xmlNodePtr cur)
{
  struct playlist *i = playlist_new_list ();

  while (cur)
    {
      if (!xmlStrcmp (cur->name, "title"))
	i->title = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
      else if (!xmlStrcmp (cur->name, "track"))
	{
	  struct playlist *p = playlist_parse_xml_track (doc, cur->xmlChildrenNode);
	  i->data.list = g_slist_append (i->data.list, p);
	}
      else if (!xmlStrcmp (cur->name, "list"))
	{
	  struct playlist *p = playlist_parse_xml (doc, cur->xmlChildrenNode);
	  i->data.list = g_slist_append (i->data.list, p);
	}
	
      cur = cur->next;
    }

  return i;
}

struct playlist *
playlist_xml_load (gchar *name)
{
  xmlDocPtr doc = xmlParseFile (name);
  xmlNodePtr cur;
  struct playlist *result;

  if (doc == NULL)
    {
      gpe_perror_box (name);
      return FALSE;
    }

  xmlCleanupParser ();

  cur = xmlDocGetRootElement (doc);
  if (cur == NULL)
    {
      gpe_error_box ("Playlist description is empty");
      xmlFreeDoc (doc);
      return FALSE;
    }

  if (xmlStrcmp (cur->name, "playlist")) 
    {
      gpe_error_box ("Playlist description has wrong document type");
      xmlFreeDoc (doc);
      return FALSE;
    }

  result = playlist_parse_xml (doc, cur->xmlChildrenNode);
      
  xmlFreeDoc (doc);

  return result;
}
