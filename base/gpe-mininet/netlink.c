/*
 * Netlink code for gpe-mininet
 *
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/uio.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "iwlib.h"
#include "netlink.h"
#include "main.h"

int
rtnl_open (void)
{
  int fd;
  struct sockaddr_nl local;
  
  fd = socket (AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (fd < 0)
    {
      perror ("socket AF_NETLINK");
      return -1;
    }

  memset (&local, 0, sizeof (local));
  local.nl_family = AF_NETLINK;
  local.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_ROUTE | RTMGRP_IPV6_ROUTE;
  
  if (bind (fd, (struct sockaddr*)&local, sizeof (local)) < 0) 
    {
      perror("Cannot bind netlink socket");
      close (fd);
      return -1;
    }

  if (fcntl (fd, F_SETFL, O_NONBLOCK))
    {
      perror ("fcntl O_NONBLOCK");
      close (fd);
      return -1;
    }

  return fd;
}

static void
rtnl_wireless_event (struct iw_event *iwe)
{
	printf("%X\n",iwe->cmd);
	if (iwe->cmd == SIOCSIWESSID)
			printf("essid %s %i\n",iwe->u.essid.pointer,iwe->len);
	if (iwe->cmd == SIOCGIWAP)
			printf("ap %2x:%2x:%2x:%2x:%2x:%2x: %i\n",iwe->u.ap_addr.sa_data[0],iwe->u.ap_addr.sa_data[1],
	iwe->u.ap_addr.sa_data[2],iwe->u.ap_addr.sa_data[3],iwe->u.ap_addr.sa_data[4],iwe->u.ap_addr.sa_data[5],
	iwe->len);
		
  // do something appropriate
}

static void
rtnl_routing_event (struct rtmsg *rtm)
{
  if (rtm->rtm_dst_len == 0)
    {
      // the default route has changed
      update_netstatus ();
    }
}

static void
rtnl_dispatch (struct sockaddr_nl *nladdr, struct nlmsghdr *h)
{
  struct ifinfomsg *ifi;
  struct stream_descr	stream;
  struct iw_event	iwe;
  int ret;

  switch (h->nlmsg_type)
    {
    case RTM_NEWLINK:
      ifi = NLMSG_DATA (h);

      /* Code is ugly, but sort of works - Jean II */

      /* Check for attributes */
      if (h->nlmsg_len > NLMSG_ALIGN(sizeof(struct ifinfomsg))) {
	int attrlen = h->nlmsg_len - NLMSG_ALIGN(sizeof(struct ifinfomsg));
	struct rtattr *attr = (void*)ifi + NLMSG_ALIGN(sizeof(struct ifinfomsg));
	
	while (RTA_OK(attr, attrlen)) {
	  /* Check if the Wireless kind */
	  if(attr->rta_type == IFLA_WIRELESS) {
	    iw_init_event_stream(&stream, 
				 (void *)attr + RTA_ALIGN(sizeof(struct rtattr)),
				 attr->rta_len - RTA_ALIGN(sizeof(struct rtattr)));
	    do
	      {
		ret = iw_extract_event_stream (&stream, &iwe);
		if (ret > 0)
		  rtnl_wireless_event (&iwe);
	      }
	    while(ret > 0);
	  }
	  attr = RTA_NEXT(attr, attrlen);
	}
      }
      break;

    case RTM_NEWROUTE:
    case RTM_DELROUTE:
      rtnl_routing_event (NLMSG_DATA (h));
      break;
    }
}

void
rtnl_process (int fd)
{
  char	buf[8192];
  struct sockaddr_nl nladdr;
  struct iovec iov = { buf, sizeof(buf) };
  int status;
  struct nlmsghdr *h;

  struct msghdr msg = {
    (void*)&nladdr, sizeof(nladdr),
    &iov,	1,
    NULL,	0,
    0
  };
  
  for (;;)
    {
      status = recvmsg (fd, &msg, 0);
      if (status <= 0)
	break;

      if (msg.msg_namelen != sizeof (nladdr)) 
	continue;

      h = (struct nlmsghdr*)buf;
      while (NLMSG_OK (h, status)) 
	{
	  rtnl_dispatch (&nladdr, h);
	  
	  h = NLMSG_NEXT (h, status);
	}
    }
}
