/*
 * usqld-conhandler.c
 * implements the connection handler for a threaded server connection
 * this thread is spawned and awaits a connect from the client using the 
 * protocol primitves.
 * 
 */

#include <pthread.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h> 
#include <string.h>
#include <pthread.h>
#include <sqlite.h>

#include "usqld-protocol.h"


void * usql_conhandler_main(UsqldConHandInit * init)
{
   int sock_fd;
   
   sock_fd = init->fd;
   free(init);
   usqld_pickle * p;
   
   
   while(USQLD_OK=usqld_recv_pickle(fd,
				    &p)){
     
   }
   
}

   
