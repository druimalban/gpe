#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h> 
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <popt.h>

#include "usqld-protocol.h"
#include "usqld-server.h"
#include "usqld-conhandler.h"
#include "xdr.h"
   

int main(int argc, char * argv[])
{
  struct sockaddr_in myaddr,from_addr;   
   int ssock_fd;
   usqld_config *conf;
   int ssop;

   int single_thread =0;
   char * database_dir = NULL;
   char c;
   
   poptContext optCon;
   
   struct poptOption cmdOptions[]= {
     {"single-thread",
      'X',
      POPT_ARG_NONE,
      &single_thread,
      0,
      "run im debugging,single thread mode"},
     {"database-dir",
      'd',
      POPT_ARG_STRING,
      &database_dir,
      0,"Data base directory"},
     POPT_AUTOHELP
     {NULL,0,0,NULL,0}
   };
      
      
   optCon = poptGetContext(NULL,argc,argv,cmdOptions,0);
   while((c= poptGetNextOpt(optCon))>=0){
   }
   
   poptFreeContext(optCon);
   if(!database_dir)
     database_dir = "/tmp/";

   printf("single-thread mode is :%s\n",(single_thread?"on":"off"));
   printf("database directory is:%s\n",database_dir);

   conf = XDR_malloc(usqld_config);
   conf->db_base_dir = strdup("/tmp/");
   
   if(-1==(ssock_fd = socket(PF_INET,SOCK_STREAM,0)))
     {
       fprintf(stderr,"couldn't create socket\n");
       exit(1);
     }


   
   myaddr.sin_family = PF_INET;
   myaddr.sin_port = htons(USQLD_SERVER_PORT);
   myaddr.sin_addr.s_addr = INADDR_ANY;
   ssop = 1;
   setsockopt(ssock_fd,SOL_SOCKET,SO_REUSEADDR,(char *)&ssop,sizeof(int));
   if(-1==bind(ssock_fd,(struct sockaddr * ) &myaddr,sizeof(myaddr)))
      {
 	fprintf(stderr,"couldn't bind socket to port\n");
	exit(1);
      }
   
   listen(ssock_fd,5);
   printf("listening on port %d\n",USQLD_SERVER_PORT); 
   while(1)
     {
	
	int csock;
	socklen_t sz = sizeof(from_addr);
	pthread_t new_thread;
	usqld_conhand_init * new_thread_init;
	csock =accept(ssock_fd, (struct sockaddr *) &from_addr,&sz);
	printf("got connection spawning thread\n");
	new_thread_init = XDR_malloc(usqld_conhand_init);
	new_thread_init->fd = csock;
	new_thread_init->config = conf;
	if(!single_thread){
	  pthread_create(&new_thread,
			 NULL,
			 (void* (*) (void*)) usqld_conhandler_main,
			 (void*)new_thread_init);
	}else{
	  usqld_conhandler_main(new_thread_init);
	}
	
	
     }
   
   
      
}
