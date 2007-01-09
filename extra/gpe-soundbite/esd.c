/*   
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <esd.h>
#include <stdlib.h>
#include <fcntl.h>

int 
sound_device_open (int mode)
{
    int rate = 16000;

    if (mode == O_RDONLY)
      return esd_record_stream_fallback (ESD_BITS16 | ESD_STEREO | ESD_STREAM | ESD_RECORD,
					 rate,
					 NULL, NULL);

    if (mode == O_WRONLY)
      return esd_play_stream_fallback (ESD_BITS16 | ESD_STEREO | ESD_STREAM | ESD_PLAY,
					 rate,
					 NULL, NULL);

    return -1;
}
