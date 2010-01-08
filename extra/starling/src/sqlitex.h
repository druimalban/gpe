/* sqlitex.h - An sqlite compatibility and translation interface.
   Copyright (C) 2009 Neal H. Walfield <neal@walfield.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef STARLING_SQLITE_H
#define STARLING_SQLITE_H

#if HAVE_SQLITE_VERSION == 2
# include <sqlite.h>
#elif HAVE_SQLITE_VERSION == 3
/* Map the sqlite2 API to the sqlite3 API, at least, the relevant
   parts.  */
# include <sqlite3.h>

# include <stdarg.h>
# include <assert.h>

typedef sqlite3 sqlite;

static inline int
sqlite_exec_printf (sqlite *db, const char *sql,
		    int (*callback)(void*,int,char**,char**), void *cookie,
		    char **errmsg, ...)
{
  va_list ap;
  va_start (ap, errmsg);

  char *s = sqlite3_vmprintf (sql, ap);
  va_end (ap);

  int ret = sqlite3_exec (db, s, callback, cookie, errmsg);
  sqlite3_free (s);

  return ret;
}

# define sqlite_freemem sqlite3_free
# define sqlite_mprintf sqlite3_mprintf
# define sqlite_exec sqlite3_exec
# define sqlite_close sqlite3_close
# define sqlite_last_insert_rowid sqlite3_last_insert_rowid
# define sqlite_busy_handler sqlite3_busy_handler

static inline sqlite *
sqlite_open(const char *dbname, int mode, char **errmsg)
{
  /* We don't support any other mode.  */
  assert (mode == 0);

  sqlite3 *db = NULL;
  int ret = sqlite3_open(dbname, &db);
  if (ret != SQLITE_OK)
    /* An error occured.  */
    {
      if (errmsg)
	/* *ERRMSG is expected to be in sqlite allocated memory.  */
	*errmsg = sqlite_mprintf ("%s", sqlite3_errmsg (db));
      sqlite3_close (db);
      db = NULL;
    }

  return db;
}



#else
# error Unsupported value of HAVE_SQLITE_VERSION.
#endif

#endif
