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

int init_pcmcia_suid();

GtkWidget *Cardinfo_Build_Objects();
void Cardinfo_Free_Objects();
void Cardinfo_Save();
void Cardinfo_Restore();

#endif
