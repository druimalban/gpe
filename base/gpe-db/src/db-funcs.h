#include <sqlite.h>

int open_db(char *dbname);
int close_db(void);
char **get_tables(int *tcount);
char **get_fields(const char *tname, int *tcount);
int db_exec(const char *sql, sqlite_callback callbackfunc,
            void *data, char **errmsg);
