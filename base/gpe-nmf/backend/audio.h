/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef AUDIO_H
#define AUDIO_H

typedef struct audio *audio_t;

extern audio_t audio_open (void);
extern int audio_write (audio_t, void *, size_t);
extern void audio_close (audio_t);
extern void audio_set_rate (audio_t, guint);

#endif
