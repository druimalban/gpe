/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include <vorbis/vorbisfile.h>

#include "stream.h"
#include "decoder_i.h"

#define OGGBUFSIZ	4096

static ov_callbacks vorbisfile_callbacks;

struct decoder_engine the_decoder;

struct vorbis_context
{
  struct decoder_engine *engine;

  struct stream *s;
  int fd;

  OggVorbis_File vf;
  vorbis_comment *vc;
  vorbis_info *vi;
  int current_section;

  gboolean bos;
  gboolean eos;

  pthread_t thread;
};

static void *
vorbis_play_loop (void *vv)
{
  int i;
  int ret;
  int old_section = -1;
  unsigned char buffer[OGGBUFSIZ];
  struct vorbis_context *v = (struct vorbis_context *)vv;

  ret = ov_open_callbacks (v, &v->vf, NULL, 0, vorbisfile_callbacks);
  if (ret < 0)
    {
      fprintf (stderr, "Couldn't open Vorbis: error %d\n", ret);
      g_free (v);
      return NULL;
    }

  while (!v->eos)
    {
      /* Read comments and audio info at the start of a logical bitstream */
      if (v->bos) 
	{
	  v->vc = ov_comment(&v->vf, -1);
	  v->vi = ov_info(&v->vf, -1);
	  v->bos = 0;
	}

      old_section = v->current_section;
      ret = ov_read (&v->vf, buffer, OGGBUFSIZ, 0,
		     2, 1, &v->current_section);

      if (ret == 0) 
	{
	  /* EOF */
	  fprintf (stderr, "At EOF\n");
	  v->eos = TRUE;
	  break;
	} 
      else if (ret == OV_HOLE) 
	{
	} 
      else if (ret < 0) 
	{
	} 
      else 
	{
	  write (v->fd, buffer, ret);

	  /* did we enter a new logical bitstream? */
	  if (old_section != v->current_section && old_section != -1) 
	    v->bos = TRUE; /* Read new headers next time through */
	}
    }

  return NULL;
}

/* ------------------- Vorbisfile Callbacks ----------------- */

static size_t 
vorbisfile_cb_read (void *ptr, size_t size, size_t nmemb, void *arg)
{
  struct vorbis_context *v = (struct vorbis_context *)arg;
  
  return stream_read (v->s, ptr, size * nmemb);
}

static int 
vorbisfile_cb_seek (void *arg, ogg_int64_t offset, int whence)
{
  return -1;
}

static long 
vorbisfile_cb_tell (void *arg)
{
  return -1;
}

static int 
vorbisfile_cb_close (void *arg)
{
  return -1;
}

static ov_callbacks vorbisfile_callbacks = 
  {
    &vorbisfile_cb_read,
    &vorbisfile_cb_seek,
    &vorbisfile_cb_close,
    &vorbisfile_cb_tell
  };

static void *
vorbis_open (struct stream *s, int fd)
{
  struct vorbis_context *v = g_malloc (sizeof (struct vorbis_context));
  int ret;

  v->engine = &the_decoder;
  v->fd = fd;
  v->s = s;
  v->eos = FALSE;
  
  ret = pthread_create (&v->thread, 0, vorbis_play_loop, v);
  if (ret < 0)
    {
      fprintf (stderr, "Vorbis: couldn't create thread\n");
      ov_clear (&v->vf);
      g_free (v);
      return NULL;
    }

  return v;
}

static void 
vorbis_close (struct vorbis_context *v)
{
}

static void
vorbis_stats (struct vorbis_context *v, struct decoder_stats *ds)
{
  ds->finished = v->eos;
}

extern gboolean playlist_fill_ogg_data (struct playlist *p);

struct decoder_engine
the_decoder = 
  {
    "vorbis",
    ".ogg",
    "audio/vorbis",
    
    vorbis_open,
    vorbis_close,
    NULL,
    NULL,
    NULL,
    vorbis_stats,
    playlist_fill_ogg_data
  };
