#include <stdlib.h>
#include <stdio.h>

#define EMPTY_RESULT_CALLBACKS 1

#include <sqlite.h>

static sqlite *mydb = NULL;

int open_db(char *dbname)
{
char *errmsg;

	mydb = NULL;
	mydb = sqlite_open(dbname,0,&errmsg);
	if (mydb == NULL) {
		fprintf(stderr,"sqlite_open() failed: '%s'\n",errmsg);
		free(errmsg);
		return -1;
	} else
		return 0;
}

int close_db(void)
{
	if (mydb != NULL) {
		sqlite_close(mydb);
		mydb=NULL;
	} else
		return -1;

return 0;
}

char **get_tables(int *tcount)
{
char **results;
int rows, columns, ret;
char *errmsg;

	if ((ret=sqlite_get_table(mydb, "select tbl_name from SQLITE_MASTER",
			&results, &rows, &columns, &errmsg)) == SQLITE_OK) {
		*tcount=rows;
		return results;
	} else {
		free(errmsg);
		*tcount=0;
		return NULL;
	}
return NULL;
}

char **get_fields(const char *tname, int *tcount)
{
char **results;
int rows, columns, ret;
char *errmsg;
char *query;


	query=(char *)malloc(128 * sizeof(char)); /* FIXME: Just allocate as much as we need */
	query[0]='\0';
	sprintf(query,"select * from %s ", tname);
	if ((ret=sqlite_get_table(mydb, query,
			&results, &rows, &columns, &errmsg)) == SQLITE_OK) {
#ifdef DEBUG
		fprintf(stderr,"rows=%d cols=%d errmsg='%s'\n",rows,columns,errmsg);
#endif
		*tcount=columns;
		free(query);
		return results;
	} else {
#ifdef DEBUG
		fprintf(stderr,"rows=%d cols=%d errmsg='%s'\n",rows,columns,errmsg);
#endif
		free(errmsg);
		*tcount=0;
		free(query);
		return NULL;
	}
return NULL;
}


int db_exec(const char *sql, sqlite_callback callbackfunc,
            void *data, char **errmsg)
{
	return sqlite_exec(mydb, sql, callbackfunc, data, errmsg);
}
