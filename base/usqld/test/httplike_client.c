#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <httplike/httplike.h>


#define TEST_PORT  6789

void packet_callback(httplike_socket * sock, httplike_packet *packet){
  int *finished;
  fprintf(stderr,"Got a packet\n");
  finished= (int*)httplike_socket_get_data(sock);
  *finished =1;
  httplike_dump_packet(packet);
}

void error_callback(httplike_socket * sock, int errcode, const char * msg){
}

int main(int argc, char ** argv){
  int finished =0,sock_fd =0;
  struct sockaddr_in saddr;   
  struct hostent * he;
  char * server_host;
  httplike_socket *sock;
  httplike_packet req;
  
  if(argc==1){
    fprintf(stderr,"httplike_client <host>");
    exit(1);
  }
  server_host=argv[1];
  saddr.sin_family = PF_INET;
  saddr.sin_port = htons(TEST_PORT);  
  saddr.sin_addr.s_addr = inet_addr(server_host);

  if(saddr.sin_addr.s_addr == INADDR_NONE)
    {
      
      if(NULL==(he=gethostbyname(server_host))|| 
	 NULL==he->h_addr){
	fprintf(stderr,"host name lookup of %s failure",server_host);
	exit(1);
      }
      
      
      memcpy(&saddr.sin_addr,
	     *he->h_addr_list,
	     sizeof(struct in_addr));
      
    }
  
  sock_fd = socket(PF_INET,SOCK_STREAM,0);
  if(sock_fd==-1){
    fprintf(stderr,"unable to allocate local socket");    
    exit(1);
  }
   
  if(0!=connect(sock_fd,(struct sockaddr*)&saddr,sizeof(saddr))){
    fprintf(stderr,"unable to connect socket to server");    
    exit(1);
  }
  
  sock =httplike_new_socket(sock_fd);

  req.operation = "usqld_app";
  req.operand = "REQUEST";
  req.version= "HTTPLIKE/0.4";
  httplike_packet_add_header(&req,"Content-type","text/plain");

  httplike_socket_set_message_func(sock,packet_callback);
  httplike_socket_set_error_func(sock,error_callback);

  httplike_socket_send_packet(sock,&req);
 
  httplike_socket_set_data(sock,(void*)&finished);
  
  do{
 
    fd_set  socks;
    int rv;
    FD_ZERO(&socks);
    FD_SET(sock->fd,&socks);
    rv =select(1,&socks,NULL,NULL,NULL);
    if(rv==1){
      httplike_pump_socket(sock);
    }else{
      fprintf(stderr,"hmm dis isn't right\n");
    }
  }while(!finished);
  printf("all done\n");
  return 0;
}
