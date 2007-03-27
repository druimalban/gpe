#ifndef _AERIAL_NETDB_H
#define _AERIAL_NETDB_H

#include "prismstumbler.h"

usernetinfo_t* get_network(char* my_bssid);
int save_network(usernetinfo_t* my_net);
int check_table();
int init_db();
int create_table();
void close_db();

#endif
