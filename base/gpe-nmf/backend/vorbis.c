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
#include "audio.h"

#define OGGBUFSIZ	4096

static ov_callbacks vorbisfile_callbacks;

struct decoder_engine the_decoder;

struct vorbis_context
{
  struct decoder_engine *engine;

  struct stream *s;
  audio_t audio;

  OggVorbis_File vf;
  vorbis_comment *vc;
  vorbis_info *vi;
  int current_section;

  unsigned int rate;
  unsigned int channels;
  unsigned long long time;
  unsigned long long total_time;

  gboolean bos;
  gboolean eos;

  pthread_t thread;
};

static void *
vorbis_play_loop (void *vv)
{
  int ret;
  int old_section = -1;
  unsigned char buffer[OGGBUFSIZ];
  struct vorbis_context *v = (struct vorbis_context *)vv;

  if (pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL))
    perror ("pthread_setcancelstate");

  if (pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL))
    perror ("pthread_setcanceltype");

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
      unsigned int bytes = 0;
      unsigned char *bufp = buffer;

      if (v->bos) 
	{
	  v->vc = ov_comment(&v->vf, -1);
	  v->vi = ov_info(&v->vf, -1);
	  v->total_time = ov_pcm_total (&v->vf, -1);
	  v->bos = 0;
	  if (v->rate != v->vi->rate)
	    {
	      v->rate = v->vi->rate;
	      audio_set_rate (v->audio, v->rate);
	    }
	  if (v->channels != v->vi->channels)
	    {
	      v->channels = v->vi->channels;
	      audio_set_channels (v->audio, v->channels);
	    }
	}

      old_section = v->current_section;
      do {
	ret = ov_read (&v->vf, bufp, OGGBUFSIZ - bytes, 0,
		       2, 1, &v->current_section);
	if (ret > 0) {
	  bytes += ret;
	  bufp += ret;
	}
      } while (bytes < OGGBUFSIZ && ret > 0);

      if (ret == 0) 
	{
	  /* EOF */
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
	  audio_write (v->audio, buffer, bytes);

	  v->time += (bytes / 4);

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
  struct vorbis_context *v = (struct vorbis_context *)arg;

  return stream_seek (v->s, offset, whence);
}

static long 
vorbisfile_cb_tell (void *arg)
{
  struct vorbis_context *v = (struct vorbis_context *)arg;

  return stream_tell (v->s);
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
vorbis_open (struct stream *s, audio_t audio)
{
  struct vorbis_context *v = g_malloc (sizeof (struct vorbis_context));
  int ret;
  
  memset (v, 0, sizeof (*v));
  v->engine = &the_decoder;
  v->audio = audio;
  v->s = s;
  v->eos = FALSE;
  v->time = 0;
  v->total_time = 0;
  v->bos = 1;
  v->channels = 0;
  v->rate = 0;
  
  ret = pthread_create (&v->thread, 0, vorbis_play_loop, v);
  if (ret < 0)
    {
      perror ("pthread_create");
      ov_clear (&v->vf);
      g_free (v);
      return NULL;
    }

  return v;
}

static void 
vorbis_close (struct vorbis_context *v)
{
  if (!v->eos)
    {
      if (pthread_cancel (v->thread))
	perror ("pthread_cancel");
    }
  if (pthread_join (v->thread, NULL))
    perror ("pthread_join");
}

static void
vorbis_stats (struct vorbis_context *v, struct decoder_stats *ds)
{
  ds->finished = v->eos;
  ds->time = v->time;
  ds->total_time = v->total_time;
  ds->rate = v->rate;
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
