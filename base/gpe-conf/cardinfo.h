#ifndef GPE_CARDINFO_H
#define GPE_CARDINFO_H

typedef struct field_t {
    char *str;
    GtkWidget *widget;
} field_t;

typedef struct flag_t {
    int val;
    GtkWidget *widget;
} flag_t;

typedef struct socket_info_t {
    int fd, o_state;
    field_t card, state, dev, io, irq, driver, type;
    flag_t cd, vcc, vpp, wp;
	GtkWidget *statusbutton;
} socket_info_t;

extern socket_info_t st[];
extern int ns;
extern const char *pcmcia_tmpcfgfile;
extern const char *pcmcia_cfgfile;
extern const char *wlan_ng_cfgfile;

int init_pcmcia_suid();
void do_reset();
void do_get_card_ident(int socket);
void do_ioctl(int chan, int cmd);

GtkWidget *Cardinfo_Build_Objects();
void Cardinfo_Free_Objects();
void Cardinfo_Save();
void Cardinfo_Restore();

#define CMD_INSERT  1
#define CMD_EJECT   2
#define CMD_SUSPEND 3
#define CMD_RESUME  4
#define CMD_RESET   5

#endif
