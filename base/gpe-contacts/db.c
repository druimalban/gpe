

#define DB_NAME

sqlite *sqlite_open(const char *dbname, int mode, char **errmsg);

int db_open(void) {
    /* open persistent connection */
    const sqlite *db=NULL;
    char *errmsg;

    if (db==NULL) {
	db=sqlite(DB_NAME,"rw",&errmsg);
	if (db==NULL) {
	    /* error! */

            return -1;
	}
    }
    return 0;
}


int db_getres(const char *query) {
    /* get only the first result of a query */
    const char *res;


}
