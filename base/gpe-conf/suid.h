#ifndef _SUID_H
#define _SUID_H

#include <stdio.h>
#include <glib.h>

/* file pointers for non-suid side and according file descriptors*/
FILE *suidout; 
int suidoutfd; 
FILE *suidin;
int suidinfd;

/* file pointer for suid side data return */
FILE *nsreturn;
int nsreturnfd;

void suidloop();
int ask_root_passwd();
int check_user_access (const char *cmd);
int check_root_passwd (const char *passwd);
gboolean no_root_passwd (void);

#endif
