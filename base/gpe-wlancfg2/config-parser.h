/*

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version. 

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Library General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#ifndef __confpars_wireless__
#define __confpars_wireless__

#include <gtk/gtk.h>
#include <stdio.h>

extern int   linenr;
extern int   sectionstart;
extern int   schemecount;

#define	LINE_NOT_PRESENT 	0x00
#define	LINE_NEW 		0xFFFE
#define L_Scheme 		0
#define L_Socket 		1
#define L_Instance 		2
#define L_HWAddress		3
#define L_Info 			4
#define L_ESSID 		5
#define L_NWID			6
#define L_Mode 			7
#define L_Channel 		8
#define L_Rate 			9
#define L_Encryption 		10
#define L_EncMode 		11
#define L_KeyFormat 		12
#define L_key1 			13
#define L_key2 			14
#define L_key3 			15
#define L_key4 			16
#define L_ActiveKey		17
#define L_iwconfig		18
#define L_iwspy			19
#define L_iwpriv		20
#define L_Frequency 		21
#define L_rts			22
#define L_frag			23
#define L_sens			24

#define MAX_SCHEMES		20
#define MAX_CONFIG_LINES	1000
#define MAX_CONFIG_VALUES       25

#define WLAN_CONFIGFILE "/etc/pcmcia/wireless.opts"
//#define WLAN_CONFIGFILE "/tmp/wireless.opts"

typedef struct {
	gint	NewScheme;
	gchar	Scheme[32];
	gchar	Socket[32];
	gchar	Instance[32];
	gchar	HWAddress[32];
	gchar	Info[64];
	gchar	ESSID[32];
	gchar	NWID[32];
	gchar	Mode[16];
	gchar	Channel[32];
	gchar	Frequency[32];
	gchar	Rate[8];
	gchar	Encryption[4];
	gchar	EncMode[16];
	gboolean KeyFormat;
	gchar	key1[64];
	gchar	key2[64];
	gchar	key3[64];
	gchar	key4[64];
	gchar	ActiveKey[4];
	gchar	iwconfig[256];
	gchar	iwspy[256];
	gchar	iwpriv[256];
	gchar	rts[32];
	gchar	frag[32];
	gchar	sens[32];
	gint	lines[MAX_CONFIG_VALUES];
	gint	scheme_start;
	gint	scheme_end;
	gint	parent_scheme_end;
} Scheme_t;



extern Scheme_t schemelist[MAX_SCHEMES];
extern char 	config_file[255][MAX_CONFIG_LINES];
extern int	config_linenr[MAX_CONFIG_LINES];
extern int	esac_line;
extern int	delete_list[MAX_SCHEMES][2];
extern int	delete_list_count;
extern int      input_file_error;
extern GtkWidget	*GPE_WLANCFG;

extern int      save_config;

int yylex(void);
int yyparse(void);
int wl_get_next_token(void);
void wl_set_inputfile(FILE *inputfile);
int parse_configfile(char* cfgname);
int write_back_configfile(char* cfgname, Scheme_t *Schemes, int scount);
int parse_input(void);

#endif
