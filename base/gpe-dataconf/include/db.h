#ifndef _GPE_DATACONF_H
#define _GPE_DATACONF_H

// some defines for access control
#define AC_NONE 		0x00
#define AC_READ 		0x01
#define AC_WRITE 		0x02
#define AC_READWRITE 	0x03

#define USQLD_DBAC_NAME "_acontrol"

#ifdef USE_USQLD
#include <usql.h>
#define sql_exec_printf(a,b,c,d,e...) usqld_exec_printf(a,b,c,d,## e)
#define sql_get_table_printf(a,b,c,d,e,f,g...) usqld_get_table_printf(a,b,c,d,e,f,## g)
#define sql_get_table(a,b,c,d,e,f) usqld_get_table(a,b,c,d,e,f)
#define sql_last_insert_rowid(a) usqld_last_insert_rowid(a)
#define sql_exec(a,b,c,d,e) usqld_exec(a,b,c,d,e)
#define sql_free_table(a) usqld_free_table(a)
#define t_sql_handle usqld_conn 
#define sql_callback usqld_callback
#else
#include <sqlite.h>
#define sql_exec_printf(a,b,c,d, e,arg...) sqlite_exec_printf(a,b,c,d,e,## arg)
#define sql_get_table_printf(a,b,c,d,e,f,g...) sqlite_get_table_printf(a,b,c,d,e,f,## g)
#define sql_get_table(a,b,c,d,e,f) sqlite_get_table(a,b,c,d,e,f)
#define sql_last_insert_rowid(a) sqlite_last_insert_rowid(a)
#define sql_exec(a,b,c,d,e) sqlite_exec(a,b,c,d,e)
#define sql_free_table(a) sqlite_free_table(a)
#define t_sql_handle sqlite 
#define sql_callback sqlite_callback
#endif

// this is the access control interface

int gpe_acontrol_set_table(t_sql_handle *sqlh, char *table, int uid, unsigned int permissions);
int gpe_acontrol_list_tables(t_sql_handle* sqlh, char*** resvec);
int gpe_acontrol_list_databases(char** resvec);
t_sql_handle* gpe_acontrol_db_open (char *dbtoopen);
unsigned int gpe_acontrol_get_table (t_sql_handle *sqlh, char *table, int uid);
int gpe_acontrol_get_list(t_sql_handle* sqlh,char* table, sql_callback cb);
void gpe_acontrol_db_close (t_sql_handle* sqlh);
int gpe_acontrol_remove_rule (t_sql_handle *sqlh, char *table, int uid);

#ifndef USE_USQLD
 #define dbpath "/.gpe/"
#endif


#endif
