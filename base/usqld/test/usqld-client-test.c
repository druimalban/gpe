#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include "usql.h"
#include "usqld-protocol.h"
#include "xdr.h"

int test_print_results(void * f,int nrows,
		       const char ** heads,
		       const char ** vals){
  int i;
  for(i = 0;i<nrows;i++){
    printf("%s\t|",heads[i]);
  }
  printf("\n");
  for(i = 0;i<nrows;i++){
    printf("%s\t|",vals[i]);
  }
  printf("\n");
  return 0;
}

int main (int argc, char ** argv) 
{
  usqld_conn * con;
  char * errstring;
  XDR_schema * s;
  
  
  s = usqld_get_protocol();
  XDR_dump_schema(s);
  if(NULL==(con=usqld_connect("localhost",
			      "test",&errstring))){
    fprintf(stderr,"couldn't connect to database: %s\n",errstring);
    exit(1);
  }
  fprintf(stderr,"connected to database\n");
  if(USQLD_OK!=
     usqld_exec(con,
		"CREATE TABLE foo(x int, y int)",
		NULL,NULL,&errstring)){
    fprintf(stderr,"couldn't query database: %s\n",errstring);

  }

  if(USQLD_OK!=
     usqld_exec(con,
		"INSERT INTO foo VALUES(1,2)",
		NULL,NULL,&errstring)){
    fprintf(stderr,"couldn't query database: %s\n",errstring);
  }

  if(USQLD_OK!=
     usqld_exec(con,
		"INSERT INTO foo VALUES(2,3)",
		NULL,NULL,&errstring)){
    fprintf(stderr,"couldn't query database: %s\n",errstring);
  }
  
  if(USQLD_OK!=
     usqld_exec(con,
		"SELECT x,y from foo",
		test_print_results,NULL,&errstring)){
    fprintf(stderr,"couldn't query database: %s\n",errstring);
    exit(1);
  }
  return 0;
}
