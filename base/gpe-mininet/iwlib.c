/*
 *	Wireless Tools
 *
 *		Jean II - HPLB 97->99 - HPL 99->03
 *
 * Common subroutines to all the wireless tools...
 *
 * This file is released under the GPL license.
 *     Copyright (c) 1997-2003 Jean Tourrilhes <jt@hpl.hp.com>
 */

/***************************** INCLUDES *****************************/
#include "iwlib.h"		/* Header */

/************************ EVENT SUBROUTINES ************************/
/*
 * The Wireless Extension API 14 and greater define Wireless Events,
 * that are used for various events and scanning.
 * Those functions help the decoding of events, so are needed only in
 * this case.
 */

/* Type of headers we know about (basically union iwreq_data) */
#define IW_HEADER_TYPE_NULL	0	/* Not available */
#define IW_HEADER_TYPE_CHAR	2	/* char [IFNAMSIZ] */
#define IW_HEADER_TYPE_UINT	4	/* __u32 */
#define IW_HEADER_TYPE_FREQ	5	/* struct iw_freq */
#define IW_HEADER_TYPE_ADDR	6	/* struct sockaddr */
#define IW_HEADER_TYPE_POINT	8	/* struct iw_point */
#define IW_HEADER_TYPE_PARAM	9	/* struct iw_param */
#define IW_HEADER_TYPE_QUAL	10	/* struct iw_quality */

/* Headers for the various requests */
static const char standard_ioctl_hdr[] = {
	IW_HEADER_TYPE_NULL,	/* SIOCSIWCOMMIT */
	IW_HEADER_TYPE_CHAR,	/* SIOCGIWNAME */
	IW_HEADER_TYPE_PARAM,	/* SIOCSIWNWID */
	IW_HEADER_TYPE_PARAM,	/* SIOCGIWNWID */
	IW_HEADER_TYPE_FREQ,	/* SIOCSIWFREQ */
	IW_HEADER_TYPE_FREQ,	/* SIOCGIWFREQ */
	IW_HEADER_TYPE_UINT,	/* SIOCSIWMODE */
	IW_HEADER_TYPE_UINT,	/* SIOCGIWMODE */
	IW_HEADER_TYPE_PARAM,	/* SIOCSIWSENS */
	IW_HEADER_TYPE_PARAM,	/* SIOCGIWSENS */
	IW_HEADER_TYPE_NULL,	/* SIOCSIWRANGE */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWRANGE */
	IW_HEADER_TYPE_NULL,	/* SIOCSIWPRIV */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWPRIV */
	IW_HEADER_TYPE_NULL,	/* SIOCSIWSTATS */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWSTATS */
	IW_HEADER_TYPE_POINT,	/* SIOCSIWSPY */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWSPY */
	IW_HEADER_TYPE_POINT,	/* SIOCSIWTHRSPY */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWTHRSPY */
	IW_HEADER_TYPE_ADDR,	/* SIOCSIWAP */
	IW_HEADER_TYPE_ADDR,	/* SIOCGIWAP */
	IW_HEADER_TYPE_NULL,	/* -- hole -- */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWAPLIST */
	IW_HEADER_TYPE_PARAM,	/* SIOCSIWSCAN */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWSCAN */
	IW_HEADER_TYPE_POINT,	/* SIOCSIWESSID */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWESSID */
	IW_HEADER_TYPE_POINT,	/* SIOCSIWNICKN */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWNICKN */
	IW_HEADER_TYPE_NULL,	/* -- hole -- */
	IW_HEADER_TYPE_NULL,	/* -- hole -- */
	IW_HEADER_TYPE_PARAM,	/* SIOCSIWRATE */
	IW_HEADER_TYPE_PARAM,	/* SIOCGIWRATE */
	IW_HEADER_TYPE_PARAM,	/* SIOCSIWRTS */
	IW_HEADER_TYPE_PARAM,	/* SIOCGIWRTS */
	IW_HEADER_TYPE_PARAM,	/* SIOCSIWFRAG */
	IW_HEADER_TYPE_PARAM,	/* SIOCGIWFRAG */
	IW_HEADER_TYPE_PARAM,	/* SIOCSIWTXPOW */
	IW_HEADER_TYPE_PARAM,	/* SIOCGIWTXPOW */
	IW_HEADER_TYPE_PARAM,	/* SIOCSIWRETRY */
	IW_HEADER_TYPE_PARAM,	/* SIOCGIWRETRY */
	IW_HEADER_TYPE_POINT,	/* SIOCSIWENCODE */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWENCODE */
	IW_HEADER_TYPE_PARAM,	/* SIOCSIWPOWER */
	IW_HEADER_TYPE_PARAM,	/* SIOCGIWPOWER */
};
static const unsigned int standard_ioctl_num = sizeof(standard_ioctl_hdr);

/*
 * Meta-data about all the additional standard Wireless Extension events
 * we know about.
 */
static const char	standard_event_hdr[] = {
	IW_HEADER_TYPE_ADDR,	/* IWEVTXDROP */
	IW_HEADER_TYPE_QUAL,	/* IWEVQUAL */
	IW_HEADER_TYPE_POINT,	/* IWEVCUSTOM */
	IW_HEADER_TYPE_ADDR,	/* IWEVREGISTERED */
	IW_HEADER_TYPE_ADDR,	/* IWEVEXPIRED */
};
static const unsigned int standard_event_num = sizeof(standard_event_hdr);

/* Size (in bytes) of various events */
static const int event_type_size[] = {
	IW_EV_LCP_LEN,		/* IW_HEADER_TYPE_NULL */
	0,
	IW_EV_CHAR_LEN,		/* IW_HEADER_TYPE_CHAR */
	0,
	IW_EV_UINT_LEN,		/* IW_HEADER_TYPE_UINT */
	IW_EV_FREQ_LEN,		/* IW_HEADER_TYPE_FREQ */
	IW_EV_ADDR_LEN,		/* IW_HEADER_TYPE_ADDR */
	0,
	IW_EV_POINT_LEN,	/* Without variable payload */
	IW_EV_PARAM_LEN,	/* IW_HEADER_TYPE_PARAM */
	IW_EV_QUAL_LEN,		/* IW_HEADER_TYPE_QUAL */
};

/*------------------------------------------------------------------*/
/*
 * Initialise the struct stream_descr so that we can extract
 * individual events from the event stream.
 */
void
iw_init_event_stream(struct stream_descr *	stream,	/* Stream of events */
		     char *			data,
		     int			len)
{
  /* Cleanup */
  memset((char *) stream, '\0', sizeof(struct stream_descr));

  /* Set things up */
  stream->current = data;
  stream->end = data + len;
}

/*------------------------------------------------------------------*/
/*
 * Extract the next event from the event stream.
 */
int
iw_extract_event_stream(struct stream_descr *	stream,	/* Stream of events */
			struct iw_event *	iwe)	/* Extracted event */
{
  int		event_type = 0;
  unsigned int	event_len = 1;		/* Invalid */
  char *	pointer;
  /* Don't "optimise" the following variable, it will crash */
  unsigned	cmd_index;		/* *MUST* be unsigned */

  /* Check for end of stream */
  if((stream->current + IW_EV_LCP_LEN) > stream->end)
    return(0);

#if 0
  printf("DBG - stream->current = %p, stream->value = %p, stream->end = %p\n",
	 stream->current, stream->value, stream->end);
#endif

  /* Extract the event header (to get the event id).
   * Note : the event may be unaligned, therefore copy... */
  memcpy((char *) iwe, stream->current, IW_EV_LCP_LEN);

#if 0
  printf("DBG - iwe->cmd = 0x%X, iwe->len = %d\n",
	 iwe->cmd, iwe->len);
#endif

  /* Check invalid events */
  if(iwe->len <= IW_EV_LCP_LEN)
    return(-1);

  /* Get the type and length of that event */
  if(iwe->cmd <= SIOCIWLAST)
    {
      cmd_index = iwe->cmd - SIOCIWFIRST;
      if(cmd_index < standard_ioctl_num)
	event_type = standard_ioctl_hdr[cmd_index];
    }
  else
    {
      cmd_index = iwe->cmd - IWEVFIRST;
      if(cmd_index < standard_event_num)
	event_type = standard_event_hdr[cmd_index];
    }
  /* Unknown events -> event_type=0 => IW_EV_LCP_LEN */
  event_len = event_type_size[event_type];

  /* Check if we know about this event */
  if(event_len <= IW_EV_LCP_LEN)
    {
      /* Skip to next event */
      stream->current += iwe->len;
      return(2);
    }
  event_len -= IW_EV_LCP_LEN;

  /* Set pointer on data */
  if(stream->value != NULL)
    pointer = stream->value;			/* Next value in event */
  else
    pointer = stream->current + IW_EV_LCP_LEN;	/* First value in event */

#if 0
  printf("DBG - event_type = %d, event_len = %d, pointer = %p\n",
	 event_type, event_len, pointer);
#endif

  /* Copy the rest of the event (at least, fixed part) */
  if((pointer + event_len) > stream->end)
    {
      /* Go to next event */
      stream->current += iwe->len;
      return(-2);
    }
  memcpy((char *) iwe + IW_EV_LCP_LEN, pointer, event_len);

  /* Skip event in the stream */
  pointer += event_len;

  /* Special processing for iw_point events */
  if(event_type == IW_HEADER_TYPE_POINT)
    {
      /* Check the length of the payload */
      if((iwe->len - (event_len + IW_EV_LCP_LEN)) > 0)
	/* Set pointer on variable part (warning : non aligned) */
	iwe->u.data.pointer = pointer;
      else
	/* No data */
	iwe->u.data.pointer = NULL;

      /* Go to next event */
      stream->current += iwe->len;
    }
  else
    {
      /* Is there more value in the event ? */
      if((pointer + event_len) <= (stream->current + iwe->len))
	/* Go to next value */
	stream->value = pointer;
      else
	{
	  /* Go to next event */
	  stream->value = NULL;
	  stream->current += iwe->len;
	}
    }
  return(1);
}
