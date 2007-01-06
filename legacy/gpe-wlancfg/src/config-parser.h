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

typedef struct {
	int	IsRow;
	char	Scheme[32];
	char	Socket[32];
	char	Instance[32];
	char	HWAddress[32];
	char	Info[64];
	char	ESSID[32];
	char	NWID[32];
	char	Mode[16];
	char	Channel[32];
	char	Frequency[32];
	char	Rate[8];
	char	Encryption[4];
	char	EncMode[16];
	gboolean KeyFormat;
	char	key1[64];
	char	key2[64];
	char	key3[64];
	char	key4[64];
	char	ActiveKey[4];
	char	iwconfig[256];
	char	iwspy[256];
	char	iwpriv[256];
	char	rts[32];
	char	frag[32];
	char	sens[32];
	unsigned int	lines[MAX_CONFIG_VALUES];
	unsigned int	scheme_start;
	unsigned int	scheme_end;
	unsigned int	parent_scheme_end;
} Scheme_t;



extern Scheme_t schemelist[MAX_SCHEMES];
extern char 	config_file[255][MAX_CONFIG_LINES];
extern int	config_linenr[MAX_CONFIG_LINES];
extern int	esac_line;
extern int	delete_list[MAX_SCHEMES][2];
extern int	delete_list_count;

int wl_get_next_token(void);
void wl_set_inputfile(FILE *inputfile);
int parse_configfile(char* cfgname);
int write_back_configfile(char* cfgname, Scheme_t *Schemes, int scount);
int parse_input(void);

#endif
