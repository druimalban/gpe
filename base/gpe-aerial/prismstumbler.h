#ifndef _PRISMSTUMBLER_H
#define _PRISMSTUMBLER_H
/***************************************************************************
                          prismstumbler.h  -  description
                             -------------------
    begin                : Wed Aug 14 2002
    copyright            : (C) 2002 by Jan Fernquist, Florian Boor
    email                : boor@unix-ag.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>

#define VERSIONSTR "prismstumbler 0.7.0"

#define MAX_BUFFER_SIZE 3000	/* Size of receive buffer */

#define FAKEAP_LEVEL 7
#define PMAX 30
#define RBUFFER_SIZE 10

#define DT_PRISM   0x01
#define DT_ORINOCO 0x02
#define DT_HOSTAP  0x04

#define PS_SOCKET "/tmp/.psintercom"
#define DHCP_COMMAND "dhcpcd -i %s &"
#define SEQ_USERNET 0xFFFF

typedef enum
{
	msg_network,
	msg_config,
	msg_gps,
	msg_command,
	msg_usernet
}
psmsgtype_t;

typedef enum
{
	C_NONE,
	C_SENDLIST,
	C_CLEARLIST,
	C_DETECT_CARD,
	C_ASSOCIATE, 
	C_SUCCESS,
	C_FAILED
}
command_t;


typedef struct
{
	float Long, Lat;
	int quality;
}
psgps_t;

typedef struct
{
	int scan;
	char device[6];
	int devtype;
	int delay;
	int filter;
	int singlechan;
	int autosend;
	char wpfile[255];
}
psconfig_t;


typedef struct
{
	command_t command;
	char paramstr[40];
	int par;	
}
pscommand_t;


typedef struct
{
	time_t when;
	char DestMac[20];
	char SrcMac[20];
	char BssId[20];
	char SSID[33];
	int hasWep;
	int isAp;
	int Channel;
	int Signal;
	int Noise;
	int FrameType;
	int speed;
	float longitude, latitude;
	int isData;
	int hasIntIV;
	int dhcp;
	unsigned char subnet[5];
	int protocol;
	int isAdHoc;
}
ScanResult_t;


typedef struct
{
	int seqnr;			// sequence number 
	int isvalid;
	char ssid[33];
	char bssid[32];
	time_t first, last;
	char type[20];
	unsigned char ip_range[5];
	char ap[16];
	unsigned long pdata, psum, pint;
	char wep_key[48];
	float longitude, latitude;
	int maxsiglevel;
	int cursiglevel;
	int speed;
	char channel;
	char ishidden;
	char ipsec;
	char pcount;		// count seen protocols
	char wep;
	char dhcp;
	char isadhoc;
	char pvec[PMAX];	// seen protocols
}
psnetinfo_t;


typedef struct
{
	char bssid[20];
	char ssid[33];
	int mode; 		// o = managed, 1 = ad-hoc
	int wep;
	int dhcp;
	int channel;
	unsigned char ip[4];
	unsigned char netmask[4];
	unsigned char gateway[4];
	char wep_key[48];
	int inrange;	
	ulong userset;
}
usernetinfo_t;

#define USET_BSSID 		0x0000
#define USET_SSID  		0x0001
#define USET_MODE 	 	0x0002
#define USET_WEP   		0x0004
#define USET_DHCP  		0x0008
#define USET_CHANNEL	0x0010
#define USET_IP			0x0020
#define USET_NETMASK	0x0040
#define USET_GATEWAY	0x0080
#define USET_WEPKEY		0x0100

typedef struct
{
	psmsgtype_t type;
	union
	{
		psnetinfo_t net;
		psconfig_t cfg;
		pscommand_t command;
		psgps_t gps;
		usernetinfo_t usernet;
	}content;
}
psmessage_t;



void HelpAndBye (void);
void psmain (int socket);
extern void update_colors ();
extern int newnet_count;
extern char device_capture[6];

void update_all (ScanResult_t *aresult, int sock);

#endif
