#include <stdio.h>

FILE *suidout;
FILE *suidin;

void suidloop();
int ask_root_passwd();
int check_user_access (const char *cmd);
int check_root_passwd (const char *passwd);
