#ifndef _GPE_DATACONF_H
#define _GPE_DATACONF_H

#include <gpe-sql.h>
#include <stdlib.h>

// some defines for access control
// this makes me work without usqld
#define AC_NONE 		0x00
#define AC_READ 		0x01
#define AC_WRITE 		0x02
#define AC_READWRITE 	0x03

int gpe_acontrol_set_table(t_sql_handle* sqlh, char *table, int uid, uint permissions);
uint gpe_acontrol_get_table (t_sql_handle *sqlh, char *table, int uid);
int gpe_acontrol_get_list(t_sql_handle* sqlh,char* table, sql_callback cb);
int gpe_acontrol_remove_rule (t_sql_handle *sqlh, char *table, int uid);


#endif
