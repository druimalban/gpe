#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
int main (int argc, char ** argv) 
{
   
   struct sockaddr_un srvaddr;
   char msgbuf[100];
   int fd;
   socklen_t len;
   
   
   fd = socket(PF_UNIX,SOCK_DGRAM,0);
   if(-1==fd)
     {
	fprintf(stderr,"erk, socket won't open\n");
	exit(1);
     }
   
   srvaddr.sun_family =AF_UNIX;
   strcpy(srvaddr.sun_path,"/tmp/usqld.sock");
   
   
   while(1)
     {
	time_t thetime;
	thetime = time(NULL);
	
	snprintf(msgbuf,100,"The time is now  %s",ctime(&thetime));
	if(-1==sendto(fd,msgbuf, strlen(msgbuf), 0,
		      (struct sockaddr *)&srvaddr, sizeof(srvaddr)))
	  {
	     fprintf(stderr,"Sending message [%s] failed\n",msgbuf);
	  }
	else
	  printf("sent %s to server\n",msgbuf);
	sleep(1);
     }
}
