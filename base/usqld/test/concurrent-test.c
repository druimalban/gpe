#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include "usql.h"
#include "usqld-protocol.h"

int main (int argc, char ** argv) 
{
   usqld_conn * con;
   char * errstring;
   if(NULL==(con=usqld_connect("localhost",
			       "test",&errstring))){
      fprintf(stderr,"couldn't connect to database: %s\n",errstring);
      exit(1);
   }

   fprintf(stderr,"connected to database\n");
   
   while(1){
     if(USQLD_OK!=
	usqld_exec(con,
		   "DELETE FROM foo where x=1;",
		   NULL,NULL,&errstring)){
       fprintf(stderr,"couldn't query database: %s\n",errstring);
     }
     
     if(USQLD_OK!=
	usqld_exec(con,
		   "INSERT INTO foo VALUES(1,2);",
		   NULL,NULL,&errstring)){
     fprintf(stderr,"couldn't query database: %s\n",errstring);
     }
   
   
     if(USQLD_OK!=
	usqld_exec(con,
		   "SELECT x,y from foo;",
		   NULL,NULL,&errstring)){
       fprintf(stderr,"1couldn't query database: %s\n",errstring);
     }
   }
   return 0;
}

