#ifndef _CFGFILE_H
#define _CFGFILE_H

#define WLAN_CONFIGFILE "/etc/pcmcia/wireless.opts"

typedef struct {
	char name[32];
	char socket[32];
	char instance[32];
	char hwaddr[32];
	int firstline;
	int lastline;
} Schemelist_t;

int set_file_open(int openon);

int write_sections( Scheme_t *Schemes, int schemescount);

#endif
