/* ogginfo
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright 2001, JAmes Atwill <ogg@linuxstuff.org>
 *
 * Portions from libvorbis examples, (c) Monty <monty@xiph.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <glib.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include "playlist_db.h"

static void 
parse_headers (vorbis_comment *vc, struct playlist *p)
{
  int i;

  for (i=0; i < vc->comments; i++) 
    {
      char *s = vc->user_comments[i];
      char *e = strchr (s, '=');
      if (e)
	{
	  e++;
	  if (!strncasecmp (s, "ARTIST", 6))
	    p->data.track.artist = g_strdup (e);
	  else if (!strncasecmp (s, "ALBUM", 5))
	    p->data.track.album = g_strdup (e);
	  else if (!strncasecmp (s, "TITLE", 5))
	    p->title = g_strdup (e);
	}
    }
}


/* Test the integrity of the stream header.  
   Return:
     1 if it is good
     0 if it is corrupted and os, vi, and vc were initialized
    -1 if it is corrupted and os, vi, and vc were not initialized 
       (don't clear them) */
static int test_header (FILE *fp, ogg_sync_state *oy, ogg_stream_state *os,
			vorbis_info *vi, vorbis_comment  *vc, long *serialno)
{
  ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
  ogg_packet       op; /* one raw packet of data for decode */
  char *buffer;
  int bytes;
  int i;
  
  /* grab some data at the head of the stream.  We want the first page
     (which is guaranteed to be small and only contain the Vorbis
     stream initial header) We need the first page to get the stream
     serialno. */
  
  /* submit a 4k block to libvorbis' Ogg layer */
  buffer=ogg_sync_buffer(oy,4096);
  bytes=fread(buffer,1,4096,fp);
  ogg_sync_wrote(oy,bytes);
  
  /* Get the first page. */
  if(ogg_sync_pageout(oy,&og)!=1){
    /* error case.  Must not be Vorbis data */
    return -1;
  }
  
  /* Get the serial number and set up the rest of decode. */
  /* serialno first; use it to set up a logical stream */
  *serialno = ogg_page_serialno(&og);
  ogg_stream_init(os, *serialno);
  
  /* extract the initial header from the first page and verify that the
     Ogg bitstream is in fact Vorbis data */
  
  /* I handle the initial header first instead of just having the code
     read all three Vorbis headers at once because reading the initial
     header is an easy way to identify a Vorbis bitstream and it's
     useful to see that functionality seperated out. */
  
  vorbis_info_init(vi);
  vorbis_comment_init(vc);
  if(ogg_stream_pagein(os,&og)<0){ 
    /* error; stream version mismatch perhaps */
    return 0;
  }
    
  if(ogg_stream_packetout(os,&op)!=1){ 
    /* no page? must not be vorbis */
    return 0;
  }
  
  if(vorbis_synthesis_headerin(vi,vc,&op)<0){ 
    /* error case; not a vorbis header */
    return 0;
  }
    
  /* At this point, we're sure we're Vorbis.  We've set up the logical
     (Ogg) bitstream decoder.  Get the comment and codebook headers and
     set up the Vorbis decoder */
  
  /* The next two packets in order are the comment and codebook headers.
     They're likely large and may span multiple pages.  Thus we reead
     and submit data until we get our two pacakets, watching that no
     pages are missing.  If a page is missing, error out; losing a
     header page is the only place where missing data is fatal. */
  
  i=0;
  while(i<2){
    while(i<2){
      int result=ogg_sync_pageout(oy,&og);
      if(result==0)break; /* Need more data */
      /* Don't complain about missing or corrupt data yet.  We'll
	 catch it at the packet output phase */
      if(result==1){
	ogg_stream_pagein(os,&og); /* we can ignore any errors here
				       as they'll also become apparent
				       at packetout */
	while(i<2){
	  result=ogg_stream_packetout(os,&op);
	  if(result==0)break;
	  if(result<0){
	    /* Uh oh; data at some point was corrupted or missing!
	       We can't tolerate that in a header. */
	    return 0;
	  }
	  vorbis_synthesis_headerin(vi,vc,&op);
	  i++;
	}
      }
    }

    /* no harm in not checking before adding more */
    buffer=ogg_sync_buffer(oy,4096);
    bytes=fread(buffer,1,4096,fp);
    if(bytes==0 && i<2){
      return 0;
    }
    ogg_sync_wrote(oy,bytes);
  }
  
  /* If we made it this far, the header must be good. */
  return 1;
}


/* Tests the integrity of a vorbis stream.  Returns 1 if the header is good
   (but not necessarily the rest of the stream) and 0 otherwise.
   
   Huge chunks of this function are from decoder_example.c (Copyright
   1994-2001 Xiphophorus Company). */
gboolean
playlist_fill_ogg_data (struct playlist *p)
{
  int header_state = -1; /* Assume the worst */

  ogg_sync_state   oy; /* sync and verify incoming physical bitstream */
  ogg_stream_state os; /* take physical pages, weld into a logical
			  stream of packets */
  vorbis_info      vi; /* struct that stores all the static vorbis bitstream
			  settings */
  vorbis_comment   vc; /* struct that stores all the bitstream user comments */
  
  FILE *fp;
  long serialno;
  gboolean rc = FALSE;

  /********** Decode setup ************/

  fp = fopen (p->data.track.url, "rb");
  if (fp == NULL)
    return FALSE;

  ogg_sync_init (&oy); /* Now we can read pages */
  
  header_state = test_header (fp, &oy, &os, &vi, &vc, &serialno);

  /* Output test results */
  if (header_state == 1) 
    {
      parse_headers (&vc, p);
      rc = TRUE;
    }
    
  if (header_state >= 0) 
    {
      /* We got far enough to initialize these structures */
      ogg_stream_clear(&os);
      
      /* ogg_page and ogg_packet structs always point to storage in
	 libvorbis.  They're never freed or manipulated directly */
      
      vorbis_comment_clear(&vc);
      vorbis_info_clear(&vi);  /* must be called last */
    }
  
  /* OK, clean up the framer */
  ogg_sync_clear(&oy);
  
  fclose (fp);

  return rc;
}
