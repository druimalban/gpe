#ifndef _GPE_SQL_H
#define _GPE_SQL_H

#define USQLD_DBAC_NAME "_acontrol"
#define USQLD_CFGDB "/etc/usqld/cfgdb"

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

// this is the interface

int sql_list_tables(t_sql_handle* sqlh, char*** resvec);
t_sql_handle* sql_open (const char *dbtoopen);
void sql_close (t_sql_handle* sqlh);

#ifndef USE_USQLD
 #define dbpath "/.gpe/"
#endif


#endif
