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

#define SERVER_PORT 8322

typedef struct 
{   int fd;
}USQLD_threadinit;

void *  child_thread(void * param)
{
   char ibuf[100],obuf[100];
   ssize_t nb=0;
   int csock;
   csock = ((USQLD_threadinit *)param)->fd;
   free(param);
   printf("new thread\n");
   
   while(1)
     {
	bzero(ibuf,100);
	if(-1!=(nb =recv(csock,ibuf,100,0))){
	   printf("got %d bytes : \"%s\" from client\n",nb,ibuf);
	   snprintf(obuf,100,"Thanks for your %d bytes (%s)\n",nb,ibuf);
	   send(csock,obuf,strlen(obuf),0);
	}else
	  {
	     pthread_exit(NULL);
	  }
     }   
}

int main(int argc, char ** argv)
{

   struct sockaddr_in myaddr,from_addr;   int ssock_fd,lock_fd;
   socklen_t len;

   if(-1==(ssock_fd = socket(PF_INET,SOCK_STREAM,0)))
     {
	fprintf(stderr,"couldn't create socket\n");
	 exit(1);
	
     }
   myaddr.sin_family = PF_INET;
   myaddr.sin_port = htons(SERVER_PORT);
   myaddr.sin_addr.s_addr = INADDR_ANY;
   if(-1==bind(ssock_fd,(struct sockaddr * ) &myaddr,sizeof(myaddr)))
      {
 	fprintf(stderr,"couldn't bind socket to port\n");
	 exit(1);
      }
   
   listen(ssock_fd,5);
   
   printf("listening on port %d\n",SERVER_PORT);
   
   while(1)
     {
	
	int csock;
	socklen_t sz = sizeof(from_addr);
	pthread_t new_thread;
	USQLD_threadinit * new_thread_init;
	csock =accept(ssock_fd, (struct sockaddr *) &from_addr,&sz);
	printf("got connection spawning thread\n");
	new_thread_init = (USQLD_threadinit *) malloc(sizeof(USQLD_threadinit));
	new_thread_init->fd = csock;
	pthread_create(&new_thread,NULL,child_thread,(void*)new_thread_init);
	
     }
   
   
      
}
