
#ifndef _USQLD_CLIENT_H_
#define _USQLD_CLIENT_H_

#include <stdarg.h>
extern char * USQLD_VERSION;
extern char * USQLD_PROTOCOL_VERSION;

typedef struct usqld_conn usqld_conn;

usqld_conn * usqld_connect(const char * server,
			   const char * database,
			   char ** errmsg);

void usqld_disconnect(usqld_conn *);

void usqld_interrupt(usqld_conn*);

int usqld_complete(const char *sql);

typedef int (*usqld_callback)(void*,int, char**,  char**);

int usqld_exec(
  usqld_conn*,                      /* An open database */
  const char *sql,              /* SQL to be executed */
  usqld_callback,              /* Callback function */
  void *,                       /* 1st argument to callback function */
  char **errmsg                 /* Error msg written here */
);
int usqld_exec_printf(
  usqld_conn*,                      /* An open database */
  const char *sqlFormat,        /* printf-style format string for the SQL */
  usqld_callback,              /* Callback function */
  void *,                       /* 1st argument to callback function */
  char **errmsg,                /* Error msg written here */
  ...                           /* Arguments to the format string. */
);
int usqld_exec_vprintf(
  usqld_conn*,                      /* An open database */
  const char *sqlFormat,        /* printf-style format string for the SQL */
  usqld_callback,              /* Callback function */
  void *,                       /* 1st argument to callback function */
  char **errmsg,                /* Error msg written here */
  va_list ap                    /* Arguments to the format string. */
);
int usqld_get_table_printf(
  usqld_conn*,               /* An open database */
  const char *sqlFormat, /* printf-style format string for the SQL */
  char ***resultp,       /* Result written to a char *[]  that this points to */
  int *nrow,             /* Number of result rows written here */
  int *ncolumn,          /* Number of result columns written here */
  char **errmsg,         /* Error msg written here */
  ...                    /* Arguments to the format string */
);
int usqld_get_table_vprintf(
  usqld_conn*,               /* An open database */
  const char *sqlFormat, /* printf-style format string for the SQL */
  char ***resultp,       /* Result written to a char *[]  that this points to */
  int *nrow,             /* Number of result rows written here */
  int *ncolumn,          /* Number of result columns written here */
  char **errmsg,         /* Error msg written here */
  va_list ap             /* Arguments to the format string */
);

int usqld_get_table(
  usqld_conn *db,                 /* The database on which the SQL executes */
  const char *zSql,           /* The SQL to be executed */
  char ***pazResult,          /* Write the result table here */
  int *pnRow,                 /* Write the number of rows in the result here */
  int *pnColumn,              /* Write the number of columns of result here */
  char **pzErrMsg             /* Write error messages here */
);
void usqld_free_table(
  char **azResult             /* Result returned from from sqlite_get_table() */
);

int usqld_last_insert_rowid(usqld_conn *con);

#define SQLITE_OK           0   /* Successful result */
#define SQLITE_ERROR        1   /* SQL error or missing database */
#define SQLITE_INTERNAL     2   /* An internal logic error in SQLite */
#define SQLITE_PERM         3   /* Access permission denied */
#define SQLITE_ABORT        4   /* Callback routine requested an abort */
#define SQLITE_BUSY         5   /* The database file is locked */
#define SQLITE_LOCKED       6   /* A table in the database is locked */
#define SQLITE_NOMEM        7   /* A malloc() failed */
#define SQLITE_READONLY     8   /* Attempt to write a readonly database */
#define SQLITE_INTERRUPT    9   /* Operation terminated by sqlite_interrupt() */

#define SQLITE_IOERR       10   /* Some kind of disk I/O error occurred */
#define SQLITE_CORRUPT     11   /* The database disk image is malformed */
#define SQLITE_NOTFOUND    12   /* (Internal Only) Table or record not found */
#define SQLITE_FULL        13   /* Insertion failed because database is full */
#define SQLITE_CANTOPEN    14   /* Unable to open the database file */
#define SQLITE_PROTOCOL    15   /* Database lock protocol error */
#define SQLITE_EMPTY       16   /* (Internal Only) Database table is empty */
#define SQLITE_SCHEMA      17   /* The database schema changed */
#define SQLITE_TOOBIG      18   /* Too much data for one row of a table */
#define SQLITE_CONSTRAINT  19   /* Abort due to contraint violation */
#define SQLITE_MISMATCH    20   /* Data type mismatch */



#endif
