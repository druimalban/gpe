#include <esd.h>
#include <stdlib.h>
#include <fcntl.h>

static char *host = "192.168.2.2";

int 
sound_device_open (int mode)
{
    int rate = 16000;

    if (mode == O_RDONLY)
      return esd_record_stream_fallback (ESD_BITS16 | ESD_STEREO | ESD_STREAM | ESD_RECORD,
					 rate,
					 host, NULL);

    if (mode == O_WRONLY)
      return esd_play_stream_fallback (ESD_BITS16 | ESD_STEREO | ESD_STREAM | ESD_PLAY,
					 rate,
					 host, NULL);

    return -1;
}
