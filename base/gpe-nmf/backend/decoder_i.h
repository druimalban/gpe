#ifndef DECODER_I_H
#define DECODER_I_H

#include "decoder.h"
#include "stream.h"
#include "audio.h"
#include "playlist_db.h"

struct decoder_engine
{
  gchar *name;
  gchar *extension;
  gchar *mime_type;

  decoder_t (*open)(stream_t in, audio_t out);
  void (*close)(decoder_t);
  gboolean (*seek)(decoder_t, unsigned long long time);
  void (*pause)(decoder_t);
  void (*unpause)(decoder_t);
  void (*stats)(decoder_t, struct decoder_stats *);
  gboolean (*fill_in_playlist)(struct playlist *);
};

#endif
