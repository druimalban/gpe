/*
** 2001 September 15
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This header file defines the interface that the Nsqlc library
** presents to client programs.
**
** @(#) $Id$
*/
#ifndef _GPE__CLIENT_H_
#define _GPE__CLIENT_H_
#include <stdarg.h>     /* Needed for the definition of va_list */

/*
** The version of the Nsqlc library.
*/
#define GPESYNC_CLIENT_VERSION         "2.7.2"

/*
** Make sure we can call this stuff from C++.
*/
#ifdef __cplusplus
extern "C" {
#endif

/*
** The version string is also compiled into the library so that a program
** can check to make sure that the lib*.a file and the *.h file are from
** the same version.
*/
extern const char gpesync_client_version[];

/*
** The GPESYNC_CLIENT_UTF8 macro is defined if the library expects to see
** UTF-8 encoded data.  The GPESYNC_CLIENT_ISO8859 macro is defined if the
** iso8859 encoded should be used.
*/
#define GPESYNC_CLIENT_ISO8859 1

/*
** The following constant holds one of two strings, "UTF-8" or "iso8859",
** depending on which character encoding the Nsqlc library expects to
** see.  The character encoding makes a difference for the LIKE and GLOB
** operators and for the LENGTH() and SUBSTR() functions.
*/
extern const char gpesync_client_encoding[];

/*
** Each open gpesync_client database is represented by an instance of the
** following opaque structure.
*/
typedef struct gpesync_client gpesync_client;

/*
** A function to open a new gpesync_client database.  
**
** If the database does not exist and mode indicates write
** permission, then a new database is created.  If the database
** does not exist and mode does not indicate write permission,
** then the open fails, an error message generated (if errmsg!=0)
** and the function returns 0.
** 
** If mode does not indicates user write permission, then the 
** database is opened read-only.
**
** The Truth:  As currently implemented, all databases are opened
** for writing all the time.  Maybe someday we will provide the
** ability to open a database readonly.  The mode parameters is
** provide in anticipation of that enhancement.
*/
gpesync_client *gpesync_client_open_ssh(const char *database, int mode, char **errmsg);

/*
** A function to close the database.
**
** Call this function with a pointer to a structure that was previously
** returned from gpesync_client_open() and the corresponding database will by closed.
*/
void gpesync_client_close(gpesync_client *);

/*
** The type for a callback function.
*/
typedef int (*gpesync_client_callback)(void*,int,char**);

/*
** A function to executes one or more statements of SQL.
**
** If one or more of the SQL statements are queries, then
** the callback function specified by the 3rd parameter is
** invoked once for each row of the query result.  This callback
** should normally return 0.  If the callback returns a non-zero
** value then the query is aborted, all subsequent SQL statements
** are skipped and the gpesync_client_exec() function returns the GPESYNC_CLIENT_ABORT.
**
** The 4th parameter is an arbitrary pointer that is passed
** to the callback function as its first parameter.
**
** The 2nd parameter to the callback function is the number of
** columns in the query result.  The 3rd parameter to the callback
** is an array of strings holding the values for each column.
** The 4th parameter to the callback is an array of strings holding
** the names of each column.
**
** The callback function may be NULL, even for queries.  A NULL
** callback is not an error.  It just means that no callback
** will be invoked.
**
** If an error occurs while parsing or evaluating the SQL (but
** not while executing the callback) then an appropriate error
** message is written into memory obtained from malloc() and
** *errmsg is made to point to that message.  The calling function
** is responsible for freeing the memory that holds the error
** message.  If errmsg==NULL, then no error message is ever written.
**
** The return value is is GPESYNC_CLIENT_OK if there are no errors and
** some other return code if there is an error.  The particular
** return value depends on the type of error. 
**
** If the query could not be executed because a database file is
** locked or busy, then this function returns GPESYNC_CLIENT_BUSY.  (This
** behavior can be modified somewhat using the gpesync_client_busy_handler()
** and gpesync_client_busy_timeout() functions below.)
*/
int gpesync_client_exec(
  gpesync_client*,                      /* An open database */
  const char *command,              /* SQL to be executed */
  gpesync_client_callback,              /* Callback function */
  void *,                       /* 1st argument to callback function */
  char **errmsg                 /* Error msg written here */
);

/*
** Return values for gpesync_client_exec()
*/
#define GPESYNC_CLIENT_OK           0   /* Successful result */
#define GPESYNC_CLIENT_ERROR        1   /* SQL error or missing database */
#define GPESYNC_CLIENT_INTERNAL     2   /* An internal logic error in Nsqlc */
#define GPESYNC_CLIENT_PERM         3   /* Access permission denied */
#define GPESYNC_CLIENT_ABORT        4   /* Callback routine requested an abort */
#define GPESYNC_CLIENT_BUSY         5   /* The database file is locked */
#define GPESYNC_CLIENT_LOCKED       6   /* A table in the database is locked */
#define GPESYNC_CLIENT_NOMEM        7   /* A malloc() failed */
#define GPESYNC_CLIENT_READONLY     8   /* Attempt to write a readonly database */
#define GPESYNC_CLIENT_INTERRUPT    9   /* Operation terminated by gpesync_client_interrupt() */
#define GPESYNC_CLIENT_IOERR       10   /* Some kind of disk I/O error occurred */
#define GPESYNC_CLIENT_CORRUPT     11   /* The database disk image is malformed */
#define GPESYNC_CLIENT_NOTFOUND    12   /* (Internal Only) Table or record not found */
#define GPESYNC_CLIENT_FULL        13   /* Insertion failed because database is full */
#define GPESYNC_CLIENT_CANTOPEN    14   /* Unable to open the database file */
#define GPESYNC_CLIENT_PROTOCOL    15   /* Database lock protocol error */
#define GPESYNC_CLIENT_EMPTY       16   /* (Internal Only) Database table is empty */
#define GPESYNC_CLIENT_SCHEMA      17   /* The database schema changed */
#define GPESYNC_CLIENT_TOOBIG      18   /* Too much data for one row of a table */
#define GPESYNC_CLIENT_CONSTRAINT  19   /* Abort due to contraint violation */
#define GPESYNC_CLIENT_MISMATCH    20   /* Data type mismatch */
#define GPESYNC_CLIENT_MISUSE      21   /* Library used incorrectly */

#ifdef __cplusplus
}  /* End of the 'extern "C"' block */
#endif

/*
** gpesync_client_mprintf() works like printf(), but allocations memory to hold the
** resulting string and returns a pointer to the allocated memory.  Use
** gpesync_clientFree() to release the memory allocated.
*/
//char *gpesync_client_mprintf(const char *zFormat, ...);
//char *gpesync_client_vmprintf(const char *zFormat, va_list ap);

/*
** The following four routines implement the varargs versions of the
** gpesync_client_exec() and gpesync_client_get_table() interfaces.  See the gpesync_client.h
** header files for a more detailed description of how these interfaces
** work.
**
** These routines are all just simple wrappers.
*/
int gpesync_client_exec_printf(
  gpesync_client *client,                   /* An open database */
  const char *query_format,        /* printf-style format string for the SQL */
  gpesync_client_callback xCallback,    /* Callback function */
  void *pArg,                   /* 1st argument to callback function */
  char **errmsg,                /* Error msg written here */
  ...                           /* Arguments to the format string. */
);

int client_callback_print (void *arg, int argc, char **argv);

int client_callback_list (void *arg, int argc, char **argv);

int client_callback_string (void *arg, int argc, char **argv);

int client_callback_gstring (void *arg, int argc, char **argv);
#endif /* _GPE__CLIENT_H_ */
