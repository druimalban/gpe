#include <unistd.h>
#include <glib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <stdio.h>

extern gboolean stop;
extern int sound_encode (int infd, int outfd);
extern int sound_decode (int infd, int outfd);
extern int sound_device_open (int mode);

void
sigint (void)
{
  stop = TRUE;
}

int
main(int argc, char *argv[])
{
  if (argc < 2)
    {
      fprintf (stderr, "must specify play or record\n");
      exit (1);
    }

  signal (SIGINT, sigint);

  if (!strcmp(argv[1], "record"))
    {
      int infd = sound_device_open (O_RDONLY);
      int outfd = open ("out.gsm", O_CREAT | O_WRONLY);
      sound_encode (infd, outfd);
    }
  if (!strcmp(argv[1], "play"))
    {
      int infd = open ("out.gsm", O_RDONLY);
      int outfd = sound_device_open (O_WRONLY);
      sound_decode (infd, outfd);
    }
  exit (0);
}
