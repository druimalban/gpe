/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 * 
 * (c) 2003 Florian Boor <florian.boor@kernelconcepts.de>
 * Some code from gpe-todo by Philip Blundell <philb@gnu.org> and
 * other gpe applications.
 *
 * This will be database client access control library soon :-)
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

#include <gpe/errorbox.h>
#include <gpe-sql.h>
#include "../include/db.h"

// define the (changable) default access permission

uint ac_default = AC_NONE;


static int
null_callback (void *arg, int argc, char **argv, char **data)
{
  if (argc > 0 && argv[0])
    {
    }

  return 0;
}

/*
	This procedure changes the user permissions for a particular table.
	Return value !=0 means that permissions could NOT be set.
*/
int 
gpe_acontrol_set_table(t_sql_handle* sqlh, char *table, int uid, uint permissions)
{
	int ret;
	char **qres;
	int nrow=0;
	int ncolumn=0;
	char *err;
	
	if (sqlh == NULL) // Check database
		return -1;
	
	ret = sql_get_table_printf(sqlh,"select amask from _acontrol where (atable='%q') and (auser=%i);",&qres,&nrow,&ncolumn,&err,table,uid);

	if (ret) {
		fprintf(stderr,"err gpe_acontrol_set_table: %s\n",err);
		free(err);
		return -1;
	}
	else
	{
		if (nrow) // alter and check
		{
			if (qres) sql_free_table(qres);
			ret = sql_get_table_printf(sqlh,"update _acontrol set amask=%i where (atable='%q') and (auser=%i);",&qres,&nrow,&ncolumn,&err,permissions,table,uid);
			if (ret) {
				fprintf(stderr,"err gpe_acontrol_set_table: %s\n",err);
				free(err);
				return -1;
			}
			if (qres) sql_free_table(qres);
		}
		else // add rule
		{
			if (qres) sql_free_table(qres);
			ret = sql_get_table_printf(sqlh,"insert into _acontrol (atable,auser,amask,aowner) values('%q',%i,%i,0);",&qres,&nrow,&ncolumn,&err,table,uid,permissions);
			if (ret) {
				fprintf(stderr,"err gpe_acontrol_set_table: %s\n",err);
				free(err);
				return -1;
			}
			if (qres) sql_free_table(qres);
			return 0;
		}
	}
	return 0; // should never get here
}


int 
gpe_acontrol_list_databases(char** resvec)
{
	int result;

	return result;
}



/*
	Returns permissions of a user on a particular table.
	In any case of an error result is AC_NONE. 
*/
uint
gpe_acontrol_get_table (t_sql_handle *sqlh, char *table, int uid)
{
  int ret;
  char **qres;
  int nrow, ncolumn;
  char *err;

  if (sqlh == NULL)	// Check database
    return AC_NONE;

  ret =
    sql_get_table_printf (sqlh,
			     "select amask from _acontrol where (atable='%q') and (auser=%i);",
			     &qres, &nrow, &ncolumn, &err, table, uid);

  if (ret)
    {
      fprintf (stderr, "err get_user_perm_table: %s", err);
      free (err);
      return AC_NONE;
    }
  else
    {
      if (nrow)
	  {
	    ret = strtoimax (qres[nrow - 1], NULL, 0);	// we'll have to do some checks here
	    sql_free_table (qres);
	    return ret;
	  }
      else
	  {
	    sql_free_table (qres);
	    return AC_NONE;
	  }
    }
}

int 
gpe_acontrol_get_list(t_sql_handle* sqlh,char* table, sql_callback cb)
{
  int ret;
  char *err;
	
  if (sqlh == NULL) return -1;
  
  ret = sql_exec_printf(sqlh,"select * from _acontrol where atable = '%q'",cb,NULL,&err,table);
  if (ret){
	  fprintf(stderr,"err gpe_acontrol_get_list: %s\n",err);
	  free(err);
  }
  return ret;
}


/*
	Removes a access control rule. 
	Should be only for frontend/testing usage. 
    returns 0 on success.
*/
int
gpe_acontrol_remove_rule (t_sql_handle *sqlh, char *table, int uid)
{
  int ret;
  char **qres;
  int nrow, ncolumn;
  char *err;

  if (sqlh == NULL)	// Check database
    return AC_NONE;

  ret =
    sql_get_table_printf (sqlh,
			     "delete from _acontrol where (atable='%q') and (auser=%i);",
			     &qres, &nrow, &ncolumn, &err, table, uid);

  if (ret)
    {
      fprintf (stderr, "err gpe_acontrol_remove_rule: %s", err);
      free (err);
      return -1;
    }
  return 0;
}
