#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h> 
#include <string.h>
#include <pthread.h>
#include <sqlite.h>
#include <assert.h>
#include <netdb.h>
#include <stdlib.h>

#include "xdr.h"
#include "usqld-protocol.h"


struct usqld_conn{
  int server_fd;
};

struct usqld_conn * usqld_connect(const char * server,
				  const char * database,
				  char ** errmsg){
  usqld_packet * p = NULL;
  XDR_tree *elems;
  
  struct sockaddr_in saddr;   
  struct usqld_conn* new_conn;
  struct hostent * he;
  int sock_fd;
  int rv;
  
  if(NULL==(he=gethostbyname(server))|| 
     NULL==he->h_addr){
    *errmsg = strdup("host name lookup failure");
    return NULL;
  }
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(USQLD_SERVER_PORT);
  inet_pton(AF_INET,"127.0.0.1",&saddr.sin_addr.s_addr);
  
  sock_fd = socket(PF_INET,SOCK_STREAM,0);
  if(sock_fd==-1){
    *errmsg = strdup("unable to allocate local socket");    
    return NULL;
  }
  
  if(0!=connect(sock_fd,(struct sockaddr*)&saddr,sizeof(saddr))){
    *errmsg = strdup("unable to connect socket to server");    
    return NULL;
  }

  
  //sock_fd = open("/tmp/op",O_WRONLY| O_CREAT,0644);
  XDR_tree_new_compound(XDR_STRUCT,
			2,
			(XDR_tree_compound**)&elems);
  
  XDR_t_set_comp_elem(elems,0,XDR_tree_new_string(USQLD_PROTOCOL_VERSION));
  XDR_t_set_comp_elem(elems,1,XDR_tree_new_string((char *)database));
  
  p = XDR_tree_new_union(PICKLE_CONNECT,
			 elems);
  
  if(USQLD_OK!=usqld_send_packet(sock_fd,
				 p)){
    *errmsg = strdup("usqld network: error while sending connect");
    XDR_tree_free(p);
    return NULL;
  }
  fprintf(stderr,"connect sent\n");
  XDR_tree_free(p);  
  fprintf(stderr,"waiting for reply\n");
  if(USQLD_OK!=(rv=usqld_recv_packet(sock_fd,
				     &p))){
    *errmsg = strdup("usqld network: error while receiving response");    
    return NULL;
  }
  fprintf(stderr,"got a reply\n");
  switch(usqld_get_packet_type(p)){
  case PICKLE_ERROR:
    {
      unsigned int errcode;
      char * error_str;
      errcode = XDR_t_get_uint(XDR_t_get_comp_elem(XDR_t_get_union_t(p),0));
      error_str = XDR_t_get_string(XDR_t_get_comp_elem(XDR_t_get_union_t(p),1));
      *errmsg = error_str;
      XDR_tree_free(p);
      return NULL;
    }
  case PICKLE_OK:
    {
      new_conn = XDR_malloc(struct usqld_conn);
      assert(new_conn!=NULL);
      new_conn->server_fd = sock_fd;
      XDR_tree_free(p);
      return new_conn;
    }
  default:
    {
      *errmsg = strdup("protocol:umm huh?");
      XDR_tree_free(p);
      return NULL;
    }
  }
  
  return NULL;
}


typedef int (*usqld_callback)(void*,int,char**, char**);

/*
  fairly icky hack probably will freeze, 
  might work
 */
int usqld_exec(
  struct usqld_conn*con,                      /* An open database */
  const char *sql,              /* SQL to be executed */
  usqld_callback cb,              /* Callback function */
  void * arg,                       /* 1st argument to callback function */
  char **errmsg                 /* Error msg written here */
  ){

  usqld_packet * p;
  int rv,complete,aborted = 0;
  char ** heads=  NULL,**rowdata = NULL;
  unsigned int nrows;

  if(con->server_fd==-1){
    *errmsg =strdup("Connection does not seem to be open");
    return SQLITE_ERROR;
  }
  
  p = XDR_tree_new_union(PICKLE_QUERY,
			 XDR_tree_new_string((char*)sql));
  
  if(USQLD_OK!=(rv=usqld_send_packet(con->server_fd,p))){
    *errmsg = strdup("usqld network: unable to send query packet");
    XDR_tree_free(p);
    return rv;
  }

  //  XDR_tree_free(p);
  
  if(USQLD_OK!=(rv=usqld_recv_packet(con->server_fd,&p))){
    *errmsg = strdup("usqld network: unable to get a response from query");
    return rv;
  }

  complete = 0;

  while(!complete){
    switch(usqld_get_packet_type(p)){
    case PICKLE_OK:
      complete =1;
      break;
    case PICKLE_STARTROWS:
      {
	int i;
	nrows = XDR_t_get_comp_len(XDR_t_get_union_t(p));
	
	heads = XDR_mallocn(char*,nrows);
	bzero(heads,sizeof(char *)*nrows);

	rowdata = XDR_mallocn(char*,nrows);
	 assert(rowdata!=NULL);
	bzero(rowdata,sizeof(char *)*nrows);
	
	for(i =0;i<nrows;i++){
	  heads[i] = strdup(XDR_t_get_string(XDR_t_get_comp_elem(XDR_t_get_union_t(p),i)));
	}	
      }
      break;
    case PICKLE_ROW:
      {

	if(!heads || !rowdata){
	  rv = USQLD_PROTOCOL_ERROR;
	  *errmsg = strdup("usqld protocol: received rows without heads!");
	  complete =1;
	  break;
	}
	
	if(nrows!=XDR_t_get_comp_len(XDR_t_get_union_t(p))){
	  rv = USQLD_PROTOCOL_ERROR;
	  *errmsg = strdup("usqld protocol: number of rows != number of heads, wierd");
	  break;
	}
	if(cb!=NULL && !aborted){ //HACK HACK HACK should actually interrupt on protocol
	  
	  int i;
	  for(i =0;i<nrows;i++){
	    rowdata[i] = XDR_t_get_string(XDR_t_get_comp_elem(XDR_t_get_union_t(p),i));
	  }
	  
	  if(0!=cb(arg,nrows,heads,rowdata)){
	    rv = SQLITE_ABORT;
	    aborted = 1;
	  }
	}
	bzero(rowdata,sizeof(char *) * nrows);
      }
      break;
     case PICKLE_ERROR:
	 {
	   complete =1;
	   *errmsg = strdup(XDR_t_get_string(
			   XDR_t_get_comp_elem(XDR_t_get_union_t(p),1)));
	   rv =XDR_t_get_uint(XDR_t_get_comp_elem(XDR_t_get_union_t(p),0));
	   
	 }
	 break;
    default:
      {
	
	rv = USQLD_PROTOCOL_ERROR;
	*errmsg = strdup("usqld protocol: huh? what");
      }
    }
    XDR_tree_free(p);
  }
  
  if(heads!=NULL){
    int i;
    for(i = 0;i<nrows;i++){
      if(heads[i]!=NULL)
	free(heads[i]);
    }
    free(heads);
  }
  
  return rv;
}
    

