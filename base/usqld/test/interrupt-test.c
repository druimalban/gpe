#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include "usql.h"
#include "usqld-protocol.h"

int cb(void *d, int nrows,const char** heads,const char **data){
  fprintf(stderr,"%s,%s\n",data[0],data[1]);
  usqld_interrupt((usqld_conn*)d);
  return 0;
}

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
   
   if(USQLD_OK!=
      usqld_exec(con,
		 "SELECT x,y from foo;",
		 cb,(void*)con,&errstring)){
     fprintf(stderr,"1couldn't query database: %s\n",errstring);
   }
   

   return 0;
}

