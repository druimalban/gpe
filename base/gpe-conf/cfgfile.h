#ifndef _CFGFILE_H
#define _CFGFILE_H

#include <gtk/gtk.h>


#define NET_CONFIGFILE "/etc/network/interfaces"
//#define NET_CONFIGFILE "/tmp/interfaces"


#define Saddress 	1
#define Snetmask	2
#define Snetwork 	3
#define Sbroadcast	4
#define Sgateway	5
#define Shostname	6
#define Sclientid 	7
#define Sprovider	8

typedef struct {
	gchar name[32];
	gchar address[32];
	gchar netmask[32];
    gchar network[32];
	gchar broadcast[32];
	gchar gateway[255];
	gchar hostname[255];
	gchar clientid[40];
	gchar provider[40];
	gint isstatic;
	gint isinet;
	gint isloop;
	gint isdhcp;
	gint isppp;
	gint firstline;
	gint lastline;
} NWInterface_t;

gint set_file_open(gint openon);
gint get_section_start(gchar* section);
gint get_section_end(gchar* section);
void empty_section(gint s_start, gint s_end);
gint write_sections();
gint read_section(gint section, NWInterface_t *Scheme);
gint rewrite_section(NWInterface_t *Scheme, gint startpos);
gint get_section_text(gchar* section);
gint get_file_text();
gint get_scheme_list();

#endif
