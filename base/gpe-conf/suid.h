#include <stdio.h>

FILE *suidout;
FILE *suidin;

void suidloop();
int ask_root_passwd();
int check_user_access (const char *cmd);
int check_root_passwd (const char *passwd);
void change_cfg_value (const gchar *file, const gchar *var, const gchar* val, gchar seperator);
