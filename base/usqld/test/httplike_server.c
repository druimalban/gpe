#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <httplike/httplike.h>

#define TEST_PORT  6789

typedef struct httplike_test_server_context{
  int stop;
  int errcode;
  char * err_msg;
}httplike_test_server_context;

void server_ctx_destructor(httplike_socket * sock,void * data){
  httplike_test_server_context * dat= (httplike_test_server_context *) data;
  assert(data !=NULL);
  fprintf (stderr,"about to destruct data: \n");
  if(dat->err_msg)
    free(dat->err_msg);
  free(dat);
}

void packet_callback(httplike_socket * sock,httplike_packet *packet){
  
  httplike_packet resp;
  char resp_data[2048];
  char * content_type_header;
  httplike_test_server_context* ctx;

  ctx = httplike_socket_get_data(sock);
  assert(ctx!=NULL);

  fprintf(stderr,"Got a packet\n");

  httplike_dump_packet(packet);
  

  if(strcmp(packet->operation,"REQ")==0){
    content_type_header = httplike_packet_get_header(packet,"Content-type");
     bzero(&resp,sizeof(resp));
      if(content_type_header && strcasecmp(content_type_header,"text/plain")==0){
	strcpy(resp_data,"<THANKYOU>");
      strcat(resp_data,packet->content);
      strcat(resp_data,"</THANKYOU>");
      
    }else{
      strcpy(resp_data,"<THANKYOU>For the data</THANKYOU>");
    }
    
    resp.content = resp_data;
    resp.content_len = strlen(resp_data);
    
    httplike_packet_add_header(&resp,"Content-type","text/plain");
    resp.operation = "usqld_app";
    resp.operand   = "REPLY";
    resp.version   = "HTTPLIKE/0.4";
    
    httplike_socket_send_packet(sock,&resp);
    httplike_packet_free_headers(&resp);
  }else if (strcmp(packet->operation,"DIE")==0){
    ctx->stop=1;
  }
  
  return;
}

void error_callback(httplike_socket * sock,int err, const char * msg){
  httplike_test_server_context * ctx;
  
  ctx = httplike_socket_get_data(sock);
  ctx->stop =1;
  ctx->errcode = err;
  ctx->err_msg = strdup(msg);  
  fprintf(stderr,"An error occured: %d : %s\n",err,msg);
  return;
}

void * httplike_client_handler(void * dat){
  httplike_socket * sock;
  httplike_test_server_context * ctx;
  sock = (httplike_socket*)dat;
  
  ctx = malloc(sizeof(httplike_test_server_context));
  assert(ctx!=NULL);
  bzero(ctx,sizeof(httplike_test_server_context));
  
  httplike_socket_set_data(sock,ctx);
  httplike_socket_set_data_destructor(sock,server_ctx_destructor);

  while(!ctx->stop){
    fd_set  socks;
    struct timeval tv;
    int rv;

    FD_ZERO(&socks);
    FD_SET(sock->fd,&socks);
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    fprintf(stderr,"waiting for something to happen\n");
    rv =select(sock->fd+1,&socks,NULL,NULL,NULL);

    if(rv==1){
      fprintf(stderr,"something just happened\n");    
      httplike_pump_socket(sock);
    }else{
      fprintf(stderr,"something unexpected just happened\n");
      return NULL;
    }
  }
  if(ctx->errcode){
    fprintf(stderr,"Dying with error %d,%s\n",ctx->errcode, ctx->err_msg);
    
  }
  httplike_socket_destroy(sock);
  return NULL;
}


int main(int argc, char ** argv){
  int ssock_fd;
  int csock,ssop;
  
  struct sockaddr_in myaddr,from_addr;  
  if(-1==(ssock_fd = socket(PF_INET,SOCK_STREAM,0)))
    {
      fprintf(stderr,"couldn't create socket\n");
      exit(1);
    }
  
  myaddr.sin_family = PF_INET;
  myaddr.sin_port = htons(TEST_PORT);
  myaddr.sin_addr.s_addr = INADDR_ANY;
  ssop = 1;
  setsockopt(ssock_fd,SOL_SOCKET,SO_REUSEADDR,(char *)&ssop,sizeof(int));

  if(-1==bind(ssock_fd,(struct sockaddr * ) &myaddr,sizeof(myaddr)))
    {
      fprintf(stderr,"couldn't bind socket to port\n");
      exit(1);
    }
  
  listen(ssock_fd,5);
  
  while(1){
    httplike_socket * my_sock ;
    pthread_t new_thread;
    socklen_t sz = sizeof(from_addr);
    csock =accept(ssock_fd, (struct sockaddr *) &from_addr,&sz);
    fprintf(stderr,"accepted connection\n");
    my_sock = httplike_new_socket(csock);
    assert(my_sock!=NULL);
#ifdef MT
    pthread_create(&new_thread,NULL,httplike_client_handler,(void *) my_sock);
#else
    httplike_client_handler((void*)my_sock);
#endif
  }
}
