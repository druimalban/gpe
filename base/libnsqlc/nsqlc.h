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
#ifndef _NSQLC_H_
#define _NSQLC_H_
#include <stdarg.h>     /* Needed for the definition of va_list */

/*
** The version of the Nsqlc library.
*/
#define NSQLC_VERSION         "2.7.2"

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
extern const char nsqlc_version[];

/*
** The NSQLC_UTF8 macro is defined if the library expects to see
** UTF-8 encoded data.  The NSQLC_ISO8859 macro is defined if the
** iso8859 encoded should be used.
*/
#define NSQLC_ISO8859 1

/*
** The following constant holds one of two strings, "UTF-8" or "iso8859",
** depending on which character encoding the Nsqlc library expects to
** see.  The character encoding makes a difference for the LIKE and GLOB
** operators and for the LENGTH() and SUBSTR() functions.
*/
extern const char nsqlc_encoding[];

/*
** Each open nsqlc database is represented by an instance of the
** following opaque structure.
*/
typedef struct nsqlc nsqlc;

/*
** A function to open a new nsqlc database.  
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
nsqlc *nsqlc_open(const char *database, int mode, char **errmsg);

/*
** A function to close the database.
**
** Call this function with a pointer to a structure that was previously
** returned from nsqlc_open() and the corresponding database will by closed.
*/
void nsqlc_close(nsqlc *);

/*
** The type for a callback function.
*/
typedef int (*nsqlc_callback)(void*,int,char**, char**);

/*
** A function to executes one or more statements of SQL.
**
** If one or more of the SQL statements are queries, then
** the callback function specified by the 3rd parameter is
** invoked once for each row of the query result.  This callback
** should normally return 0.  If the callback returns a non-zero
** value then the query is aborted, all subsequent SQL statements
** are skipped and the nsqlc_exec() function returns the NSQLC_ABORT.
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
** The return value is is NSQLC_OK if there are no errors and
** some other return code if there is an error.  The particular
** return value depends on the type of error. 
**
** If the query could not be executed because a database file is
** locked or busy, then this function returns NSQLC_BUSY.  (This
** behavior can be modified somewhat using the nsqlc_busy_handler()
** and nsqlc_busy_timeout() functions below.)
*/
int nsqlc_exec(
  nsqlc*,                      /* An open database */
  const char *sql,              /* SQL to be executed */
  nsqlc_callback,              /* Callback function */
  void *,                       /* 1st argument to callback function */
  char **errmsg                 /* Error msg written here */
);

/*
** Return values for nsqlc_exec()
*/
#define NSQLC_OK           0   /* Successful result */
#define NSQLC_ERROR        1   /* SQL error or missing database */
#define NSQLC_INTERNAL     2   /* An internal logic error in Nsqlc */
#define NSQLC_PERM         3   /* Access permission denied */
#define NSQLC_ABORT        4   /* Callback routine requested an abort */
#define NSQLC_BUSY         5   /* The database file is locked */
#define NSQLC_LOCKED       6   /* A table in the database is locked */
#define NSQLC_NOMEM        7   /* A malloc() failed */
#define NSQLC_READONLY     8   /* Attempt to write a readonly database */
#define NSQLC_INTERRUPT    9   /* Operation terminated by nsqlc_interrupt() */
#define NSQLC_IOERR       10   /* Some kind of disk I/O error occurred */
#define NSQLC_CORRUPT     11   /* The database disk image is malformed */
#define NSQLC_NOTFOUND    12   /* (Internal Only) Table or record not found */
#define NSQLC_FULL        13   /* Insertion failed because database is full */
#define NSQLC_CANTOPEN    14   /* Unable to open the database file */
#define NSQLC_PROTOCOL    15   /* Database lock protocol error */
#define NSQLC_EMPTY       16   /* (Internal Only) Database table is empty */
#define NSQLC_SCHEMA      17   /* The database schema changed */
#define NSQLC_TOOBIG      18   /* Too much data for one row of a table */
#define NSQLC_CONSTRAINT  19   /* Abort due to contraint violation */
#define NSQLC_MISMATCH    20   /* Data type mismatch */
#define NSQLC_MISUSE      21   /* Library used incorrectly */

/*
** Each entry in an Nsqlc table has a unique integer key.  (The key is
** the value of the INTEGER PRIMARY KEY column if there is such a column,
** otherwise the key is generated at random.  The unique key is always
** available as the ROWID, OID, or _ROWID_ column.)  The following routine
** returns the integer key of the most recent insert in the database.
**
** This function is similar to the mysql_insert_id() function from MySQL.
*/
int nsqlc_last_insert_rowid(nsqlc*);

/*
** This function returns the number of database rows that were changed
** (or inserted or deleted) by the most recent called nsqlc_exec().
**
** All changes are counted, even if they were later undone by a
** ROLLBACK or ABORT.  Except, changes associated with creating and
** dropping tables are not counted.
**
** If a callback invokes nsqlc_exec() recursively, then the changes
** in the inner, recursive call are counted together with the changes
** in the outer call.
**
** Nsqlc implements the command "DELETE FROM table" without a WHERE clause
** by dropping and recreating the table.  (This is much faster than going
** through and deleting individual elements form the table.)  Because of
** this optimization, the change count for "DELETE FROM table" will be
** zero regardless of the number of elements that were originally in the
** table. To get an accurate count of the number of rows deleted, use
** "DELETE FROM table WHERE 1" instead.
*/
int nsqlc_changes(nsqlc*);

/* If the parameter to this routine is one of the return value constants
** defined above, then this routine returns a constant text string which
** descripts (in English) the meaning of the return value.
*/
const char *nsqlc_error_string(int);
#define nsqlcErrStr nsqlc_error_string  /* Legacy. Do not use in new code. */

/* This function causes any pending database operation to abort and
** return at its earliest opportunity.  This routine is typically
** called in response to a user action such as pressing "Cancel"
** or Ctrl-C where the user wants a long query operation to halt
** immediately.
*/
void nsqlc_interrupt(nsqlc*);


/* This function returns true if the given input string comprises
** one or more complete SQL statements.
**
** The algorithm is simple.  If the last token other than spaces
** and comments is a semicolon, then return true.  otherwise return
** false.
*/
int nsqlc_complete(const char *sql);

/*
** This routine identifies a callback function that is invoked
** whenever an attempt is made to open a database table that is
** currently locked by another process or thread.  If the busy callback
** is NULL, then nsqlc_exec() returns NSQLC_BUSY immediately if
** it finds a locked table.  If the busy callback is not NULL, then
** nsqlc_exec() invokes the callback with three arguments.  The
** second argument is the name of the locked table and the third
** argument is the number of times the table has been busy.  If the
** busy callback returns 0, then nsqlc_exec() immediately returns
** NSQLC_BUSY.  If the callback returns non-zero, then nsqlc_exec()
** tries to open the table again and the cycle repeats.
**
** The default busy callback is NULL.
**
** Nsqlc is re-entrant, so the busy handler may start a new query. 
** (It is not clear why anyone would every want to do this, but it
** is allowed, in theory.)  But the busy handler may not close the
** database.  Closing the database from a busy handler will delete 
** data structures out from under the executing query and will 
** probably result in a coredump.
*/
void nsqlc_busy_handler(nsqlc*, int(*)(void*,const char*,int), void*);

/*
** This routine sets a busy handler that sleeps for a while when a
** table is locked.  The handler will sleep multiple times until 
** at least "ms" milleseconds of sleeping have been done.  After
** "ms" milleseconds of sleeping, the handler returns 0 which
** causes nsqlc_exec() to return NSQLC_BUSY.
**
** Calling this routine with an argument less than or equal to zero
** turns off all busy handlers.
*/
void nsqlc_busy_timeout(nsqlc*, int ms);

/*
** This next routine is really just a wrapper around nsqlc_exec().
** Instead of invoking a user-supplied callback for each row of the
** result, this routine remembers each row of the result in memory
** obtained from malloc(), then returns all of the result after the
** query has finished. 
**
** As an example, suppose the query result where this table:
**
**        Name        | Age
**        -----------------------
**        Alice       | 43
**        Bob         | 28
**        Cindy       | 21
**
** If the 3rd argument were &azResult then after the function returns
** azResult will contain the following data:
**
**        azResult[0] = "Name";
**        azResult[1] = "Age";
**        azResult[2] = "Alice";
**        azResult[3] = "43";
**        azResult[4] = "Bob";
**        azResult[5] = "28";
**        azResult[6] = "Cindy";
**        azResult[7] = "21";
**
** Notice that there is an extra row of data containing the column
** headers.  But the *nrow return value is still 3.  *ncolumn is
** set to 2.  In general, the number of values inserted into azResult
** will be ((*nrow) + 1)*(*ncolumn).
**
** After the calling function has finished using the result, it should 
** pass the result data pointer to nsqlc_free_table() in order to 
** release the memory that was malloc-ed.  Because of the way the 
** malloc() happens, the calling function must not try to call 
** malloc() directly.  Only nsqlc_free_table() is able to release 
** the memory properly and safely.
**
** The return value of this routine is the same as from nsqlc_exec().
*/
int nsqlc_get_table(
  nsqlc*,               /* An open database */
  const char *sql,       /* SQL to be executed */
  char ***resultp,       /* Result written to a char *[]  that this points to */
  int *nrow,             /* Number of result rows written here */
  int *ncolumn,          /* Number of result columns written here */
  char **errmsg          /* Error msg written here */
);

/*
** Call this routine to free the memory that nsqlc_get_table() allocated.
*/
void nsqlc_free_table(char **result);

/*
** The following routines are wrappers around nsqlc_exec() and
** nsqlc_get_table().  The only difference between the routines that
** follow and the originals is that the second argument to the 
** routines that follow is really a printf()-style format
** string describing the SQL to be executed.  Arguments to the format
** string appear at the end of the argument list.
**
** All of the usual printf formatting options apply.  In addition, there
** is a "%q" option.  %q works like %s in that it substitutes a null-terminated
** string from the argument list.  But %q also doubles every '\'' character.
** %q is designed for use inside a string literal.  By doubling each '\''
** character it escapes that character and allows it to be inserted into
** the string.
**
** For example, so some string variable contains text as follows:
**
**      char *zText = "It's a happy day!";
**
** We can use this text in an SQL statement as follows:
**
**      nsqlc_exec_printf(db, "INSERT INTO table VALUES('%q')",
**          callback1, 0, 0, zText);
**
** Because the %q format string is used, the '\'' character in zText
** is escaped and the SQL generated is as follows:
**
**      INSERT INTO table1 VALUES('It''s a happy day!')
**
** This is correct.  Had we used %s instead of %q, the generated SQL
** would have looked like this:
**
**      INSERT INTO table1 VALUES('It's a happy day!');
**
** This second example is an SQL syntax error.  As a general rule you
** should always use %q instead of %s when inserting text into a string 
** literal.
*/
int nsqlc_exec_printf(
  nsqlc*,                      /* An open database */
  const char *sqlFormat,        /* printf-style format string for the SQL */
  nsqlc_callback,              /* Callback function */
  void *,                       /* 1st argument to callback function */
  char **errmsg,                /* Error msg written here */
  ...                           /* Arguments to the format string. */
);
int nsqlc_exec_vprintf(
  nsqlc*,                      /* An open database */
  const char *sqlFormat,        /* printf-style format string for the SQL */
  nsqlc_callback,              /* Callback function */
  void *,                       /* 1st argument to callback function */
  char **errmsg,                /* Error msg written here */
  va_list ap                    /* Arguments to the format string. */
);
int nsqlc_get_table_printf(
  nsqlc*,               /* An open database */
  const char *sqlFormat, /* printf-style format string for the SQL */
  char ***resultp,       /* Result written to a char *[]  that this points to */
  int *nrow,             /* Number of result rows written here */
  int *ncolumn,          /* Number of result columns written here */
  char **errmsg,         /* Error msg written here */
  ...                    /* Arguments to the format string */
);
int nsqlc_get_table_vprintf(
  nsqlc*,               /* An open database */
  const char *sqlFormat, /* printf-style format string for the SQL */
  char ***resultp,       /* Result written to a char *[]  that this points to */
  int *nrow,             /* Number of result rows written here */
  int *ncolumn,          /* Number of result columns written here */
  char **errmsg,         /* Error msg written here */
  va_list ap             /* Arguments to the format string */
);
char *nsqlc_mprintf(const char*,...);

/*
** Windows systems should call this routine to free memory that
** is returned in the in the errmsg parameter of nsqlc_open() when
** Nsqlc is a DLL.  For some reason, it does not work to call free()
** directly.
*/
void nsqlc_freemem(void *p);

/*
** Windows systems need functions to call to return the nsqlc_version
** and nsqlc_encoding strings.
*/
const char *nsqlc_libversion(void);
const char *nsqlc_libencoding(void);

/*
** A pointer to the following structure is used to communicate with
** the implementations of user-defined functions.
*/
typedef struct nsqlc_func nsqlc_func;

/*
** Use the following routines to create new user-defined functions.  See
** the documentation for details.
*/
int nsqlc_create_function(
  nsqlc*,                  /* Database where the new function is registered */
  const char *zName,        /* Name of the new function */
  int nArg,                 /* Number of arguments.  -1 means any number */
  void (*xFunc)(nsqlc_func*,int,const char**),  /* C code to implement */
  void *pUserData           /* Available via the nsqlc_user_data() call */
);
int nsqlc_create_aggregate(
  nsqlc*,                  /* Database where the new function is registered */
  const char *zName,        /* Name of the function */
  int nArg,                 /* Number of arguments */
  void (*xStep)(nsqlc_func*,int,const char**), /* Called for each row */
  void (*xFinalize)(nsqlc_func*),       /* Called once to get final result */
  void *pUserData           /* Available via the nsqlc_user_data() call */
);

/*
** Use the following routine to define the datatype returned by a
** user-defined function.  The second argument can be one of the
** constants NSQLC_NUMERIC, NSQLC_TEXT, or NSQLC_ARGS or it
** can be an integer greater than or equal to zero.  The datatype
** will be numeric or text (the only two types supported) if the
** argument is NSQLC_NUMERIC or NSQLC_TEXT.  If the argument is
** NSQLC_ARGS, then the datatype is numeric if any argument to the
** function is numeric and is text otherwise.  If the second argument
** is an integer, then the datatype of the result is the same as the
** parameter to the function that corresponds to that integer.
*/
int nsqlc_function_type(
  nsqlc *db,               /* The database there the function is registered */
  const char *zName,        /* Name of the function */
  int datatype              /* The datatype for this function */
);
#define NSQLC_NUMERIC     (-1)
#define NSQLC_TEXT        (-2)
#define NSQLC_ARGS        (-3)

/*
** The user function implementations call one of the following four routines
** in order to return their results.  The first parameter to each of these
** routines is a copy of the first argument to xFunc() or xFinialize().
** The second parameter to these routines is the result to be returned.
** A NULL can be passed as the second parameter to nsqlc_set_result_string()
** in order to return a NULL result.
**
** The 3rd argument to _string and _error is the number of characters to
** take from the string.  If this argument is negative, then all characters
** up to and including the first '\000' are used.
**
** The nsqlc_set_result_string() function allocates a buffer to hold the
** result and returns a pointer to this buffer.  The calling routine
** (that is, the implmentation of a user function) can alter the content
** of this buffer if desired.
*/
char *nsqlc_set_result_string(nsqlc_func*,const char*,int);
void nsqlc_set_result_int(nsqlc_func*,int);
void nsqlc_set_result_double(nsqlc_func*,double);
void nsqlc_set_result_error(nsqlc_func*,const char*,int);

/*
** The pUserData parameter to the nsqlc_create_function() and
** nsqlc_create_aggregate() routines used to register user functions
** is available to the implementation of the function using this
** call.
*/
void *nsqlc_user_data(nsqlc_func*);

/*
** Aggregate functions use the following routine to allocate
** a structure for storing their state.  The first time this routine
** is called for a particular aggregate, a new structure of size nBytes
** is allocated, zeroed, and returned.  On subsequent calls (for the
** same aggregate instance) the same buffer is returned.  The implementation
** of the aggregate can use the returned buffer to accumulate data.
**
** The buffer allocated is freed automatically be Nsqlc.
*/
void *nsqlc_aggregate_context(nsqlc_func*, int nBytes);

/*
** The next routine returns the number of calls to xStep for a particular
** aggregate function instance.  The current call to xStep counts so this
** routine always returns at least 1.
*/
int nsqlc_aggregate_count(nsqlc_func*);

/*
** Attempt to open the file named in the argument as the auxiliary database
** file.  The auxiliary database file is used to store TEMP tables.  But
** by using this API, it is possible to trick Nsqlc into opening two
** separate databases and acting on them as if they were one.
*/
int nsqlc_open_aux_file(nsqlc *db, const char *zName, char **pzErrMsg);

#ifdef __cplusplus
}  /* End of the 'extern "C"' block */
#endif

#endif /* _NSQLC_H_ */
