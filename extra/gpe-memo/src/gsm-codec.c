/*   
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2002 Moray Allan <moray@sermisy.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <unistd.h>
#ifdef HAVE_GSM_H
# include <gsm.h>
# elif HAVE_GSM_GSM_H
#  include <gsm/gsm.h>
# else
#  error Can't find gsm.h
#endif
#include <glib.h>
#include <string.h>

gboolean stop;

#define NSAMP 160

/*
 * The GSM codec requires complete frames.  When reading from the network,
 * short reads can occur: this function coalesces them together.  When reading
 * from a local device this will be equivalent to plain read().
 */
size_t
cooked_read (int infd, void *buf, size_t bufsiz)
{
  int n;
  size_t bytes = 0;

  do {
    n = read (infd, buf + bytes, bufsiz - bytes);
    if (n < 0)
      return n;
    bytes += n;
  } while (bytes < bufsiz && n);

  return bytes;
}

int
sound_encode (int infd, int outfd)
{
  gshort indata[NSAMP];
  gushort indata1[2*NSAMP];
  gsm_frame outdata;
  size_t r;
  gsm g = gsm_create ();

  stop = FALSE;

  do {
    guint i;

    r = cooked_read (infd, indata1, sizeof (indata1));
    if (r < 0)
      {
	gsm_destroy (g);
	return -1;
      }

    if (r < sizeof (indata1))
      memset (indata1 + r, 0, sizeof(indata1) - r);

    for (i = 0; i < NSAMP; i++)
      indata[i] = indata1[2*i];

    gsm_encode (g, indata, outdata);
    if (write (outfd, outdata, sizeof (outdata)) < sizeof (outdata))
      {
	gsm_destroy (g);
	return -1;
      }
  } while (r == sizeof (indata1) && !stop);


  gsm_destroy (g);

  return 0;
}

int
sound_decode (int infd, int outfd)
{
  gshort outdata[NSAMP];
  gushort outdata1[NSAMP * 2];
  gsm_frame indata;
  size_t r;
  gsm g = gsm_create ();

  stop = FALSE;

  do {
    guint i;

    r = read (infd, indata, sizeof (indata));
    if (r <= 0)
      {
	gsm_destroy (g);
	return r ? -1 : 0;
      }

    if (r < sizeof (indata))
      memset (indata + r, 0, sizeof(indata) - r);

    gsm_decode (g, indata, outdata);

    for (i = 0; i < NSAMP; i++)
      {
	outdata1[2*i] = outdata[i];
	outdata1[2*i+1] = outdata[i];
      }
    if (write (outfd, outdata1, sizeof (outdata1)) < sizeof (outdata1))
      {
	gsm_destroy (g);
	return -1;
      }
  } while (!stop);

  gsm_destroy (g);

  close (infd);
  close (outfd);

  return 0;
}
