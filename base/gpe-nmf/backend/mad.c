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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <mad.h>

#include "stream.h"
#include "decoder_i.h"
#include "audio.h"

#define MADBUFSIZ	8192

struct mad_context
{
  struct decoder_engine *engine;

  struct stream *s;
  audio_t audio;
  player_t p;

  pthread_t thread;

  struct mad_decoder decoder;

  gboolean finished;
  size_t bufsiz;

  unsigned long bytes_read;
  unsigned int rate;
  unsigned int channels;
  unsigned long long time;
  unsigned long long total_time;

  char buffer[MADBUFSIZ];
};

struct decoder_engine the_decoder;

/*
 * This is the input callback. The purpose of this callback is to (re)fill
 * the stream buffer which is to be decoded. In this example, an entire file
 * has been mapped into memory, so we just call mad_stream_buffer() with the
 * address and length of the mapping. When this callback is called a second
 * time, we are finished decoding.
 */

static
enum mad_flow input(void *data,
		    struct mad_stream *stream)
{
  struct mad_context *c = (struct mad_context *)data;
  int l;
  unsigned int posn; 
  size_t bytes, old_bytes;

  if (stream->next_frame)
    {
      posn = (void *)stream->next_frame - (void *)c->buffer;
      memmove (c->buffer, c->buffer + posn, c->bufsiz - posn);
      old_bytes = (c->bufsiz - posn);
      bytes = posn;
    }
  else
    {
      bytes = MADBUFSIZ;
      old_bytes = 0;
    }

  l = stream_read (c->s, c->buffer + old_bytes, bytes);
  if (l == 0)
    return MAD_FLOW_STOP;

  c->bytes_read += l;
  c->bufsiz = old_bytes + l;
  mad_stream_buffer (stream, c->buffer, c->bufsiz);

  return MAD_FLOW_CONTINUE;
}

/*
 * The following utility routine performs simple rounding, clipping, and
 * scaling of MAD's high-resolution samples down to 16 bits. It does not
 * perform any dithering or noise shaping, which would be recommended to
 * obtain any exceptional audio quality. It is therefore not recommended to
 * use this routine if high-quality output is desired.
 */

static inline
signed int scale(mad_fixed_t sample)
{
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}

/*
 * This is the output callback function. It is called after each frame of
 * MPEG audio data has been completely decoded. The purpose of this callback
 * is to output (or play) the decoded PCM audio.
 */

#define OBUFSIZ	1024

static
enum mad_flow output(void *data,
		     struct mad_header const *header,
		     struct mad_pcm *pcm)
{
  unsigned int nchannels, nsamples;
  mad_fixed_t const *left_ch, *right_ch;
  struct mad_context *c = (struct mad_context *)data;
  unsigned short obuf[OBUFSIZ];
  unsigned int op = 0;

  pthread_testcancel ();

  /* pcm->samplerate contains the sampling frequency */

  nchannels = pcm->channels;
  nsamples  = pcm->length;
  left_ch   = pcm->samples[0];
  right_ch  = pcm->samples[1];

  if (pcm->samplerate != c->rate)
    {
      c->rate = pcm->samplerate;
      audio_set_rate (c->audio, pcm->samplerate);
    }
  if (nchannels != c->channels)
    {
      c->channels = nchannels;
      audio_set_channels (c->audio, nchannels);
    }

  c->time += nsamples / nchannels;

  while (nsamples--) {
    signed int sample;

    /* output sample(s) in 16-bit signed little-endian PCM */

    sample = scale(*left_ch++);
    obuf[op++] = sample;

    if (nchannels == 2) {
      sample = scale(*right_ch++);
      obuf[op++] = sample;
    }

    if (op == OBUFSIZ)
      {
	audio_write (c->audio, obuf, op * 2);
	op = 0;
      }
  }

  if (op)
    audio_write (c->audio, obuf, op * 2);

  return MAD_FLOW_CONTINUE;
}

/*
 * This is the error callback function. It is called whenever a decoding
 * error occurs. The error is indicated by stream->error; the list of
 * possible MAD_ERROR_* errors can be found in the mad.h (or
 * libmad/stream.h) header file.
 */

static
enum mad_flow error(void *data,
		    struct mad_stream *stream,
		    struct mad_frame *frame)
{
  struct mad_context *c = (struct mad_context *)data;
  fprintf(stderr, "decoding error 0x%04x (%s) at byte offset %lu\n",
	  stream->error, mad_stream_errorstr(stream),
	  (void *)stream->this_frame - (void *)c->buffer + c->bytes_read);

  return MAD_FLOW_CONTINUE;
}

static void *
mad_play_loop (void *d)
{
  struct mad_context *c = (struct mad_context *)d;

  if (pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL))
    perror ("pthread_setcancelstate");

  if (pthread_setcanceltype (PTHREAD_CANCEL_DEFERRED, NULL))
    perror ("pthread_setcanceltype");

  do {
    c->finished = FALSE;
    mad_decoder_init (&c->decoder, c,
		      input, 0 /* header */, 0 /* filter */, output,
		      error, 0 /* message */);
    
    /* start decoding */
    mad_decoder_run(&c->decoder, MAD_DECODER_MODE_SYNC);

    mad_decoder_finish(&c->decoder);
    c->finished = TRUE;

    c->s = player_next_stream (c->p);
  } while (c->s);
    
  return NULL;
}

static void *
mad_open (player_t p, struct stream *s, audio_t audio)
{
  struct mad_context *c = g_malloc (sizeof (struct mad_context));
  int ret;

  c->engine = &the_decoder;
  c->audio = audio;
  c->s = s;
  c->finished = FALSE;
  c->bytes_read = 0;
  c->time = c->total_time = 0;
  c->channels = 0;
  c->rate = 0;
  c->p = p;

  ret = pthread_create (&c->thread, 0, mad_play_loop, c);
  if (ret < 0)
    {
      g_free (c);
      return NULL;
    }

  return c;
}

static void 
mad_close (struct mad_context *c)
{
  if (pthread_cancel (c->thread))
    perror ("pthread_cancel");
  if (pthread_join (c->thread, NULL))
    perror ("pthread_join");

  /* release the decoder */
  if (!c->finished)
    mad_decoder_finish(&c->decoder);
}

static void
mad_stats (struct mad_context *c, struct decoder_stats *ds)
{
  ds->finished = c->finished;
  ds->rate = c->rate;
  ds->time = c->time;
  ds->total_time = c->total_time;
}

extern gboolean playlist_fill_id3_data (struct playlist *);

struct decoder_engine
the_decoder = 
  {
    "mad",
    ".mp3",
    "audio/mpeg",
    
    mad_open,
    mad_close,
    NULL,
    NULL,
    NULL,
    mad_stats,
    playlist_fill_id3_data
  };
