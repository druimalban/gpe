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

#include "usqld-conhandler.h"
#include "usqld-protocol.h"
typedef struct{
  int client_fd;
  sqlite * db;
  char * database_name;
  usqld_config * config;
}usqld_tc;

int  usqd_do_connect(usqld_tc * tc,usqld_packet * p){
  char * db = NULL;
  char * version = NULL;
  XDR_tree * content;
  usqld_packet * reply = NULL;
  int rv;
  char * errmsg = NULL;

  char fn_buf[512];
  if(tc->db!=NULL){
    reply = usqld_error_packet(SQLITE_CANTOPEN,"database already open");
    goto connect_send_reply;
  }
  content = XDR_t_get_union_t(p);
  version = XDR_t_get_comp_elem(content,0);
  database = XDR_t_get_comp_elem(content,1);
  if(strcmp(version,USQLD_PROTOCOL_VERSION)!=){
    char buf[256];
    snprintf(buf,256,"Your client's protocol version (%s) does not match"\
	     " the server version (%s)",version,USQLD_PROTOCOL_VERSION);
    
    reply = usqld_error_packet(USQLD_VERSION_MISMATCH,buf);
    goto connect_send_reply;
  }
  strncpy(512,fn_buf,tc->config->db_base_dir);
  strcat(fn_buf,database);
  
  if(NULL==(tc->db=sqlite_open(fn_buf,0644,&errmsg))){
    reply = usqld_error_packet(SQLITE_CANTOPEN,errmsg);
    goto connect_send_reply;
  }
  tc->database = strcpy(database);
  /*we are set*/
  reply = usqld_ok_packet();
  goto connect_send_reply;
 connect_send_reply:
  assert(reply!=NULL);
  rv =  usqld_send_packet(reply);
  XDR_tree_free(reply);
  return rv; 
}

typedef struct{
  usqld_tc * tc;
  int headsent;
  int rv;
}usqld_row_context;

int usqld_send_row(usqld_row_context * rc,
		   int nfields,
		   char ** heads,
		   char ** fields){
  XDR_tree * rowpacket = NULL,*field_elems;
  int i;
    
  if(!rc->headsent){
    XDR_tree  * srpacket = NULL, *head_elems;

    XDR_tree_new_compound(XDR_VARARRAY,
			  nfields,
			  &head_elems);
    for( i =0;i<nfields;i++){
      XDR_t_get_comp_elem(head_elems,i) = XDR_tree_new_string(heads[i]);
    }
    
    
    srpacket = XDR_tree_new_union(PICKLE_STARTROWS,
				  head_elems);
    if(USQLD_OK!=(rv=usqld_send_packet(tc->client_fd,
				       srpacket))){
      XDR_tree_free(srpacket);
      rc->rv = rv;
      return -1;			 
    }
    XDR_tree_free(srpacket);
    rc->headsent = 1;
  }
  
  XDR_tree_new_compound(XDR_VARARRAY,
			nfields,
			&field_elems);
  
  for(int i = 0;i<nfields;i++){
    XDR_t_get_comp_elem(field_elems,i) = XDR_tree_new_string(fields[i]);
  }
  rowpacket = XDR_tree_new_union(PICKLE_ROW,
				 field_elems);

  if(USQLD_OK!=(rv=usqld_send_packet(tc->client_fd,
				     srpacket))){
      XDR_tree_free(srpacket);
      rc->rv = rv;
      return -1;			 
      
  }
  XDR_tree_free(srpacket);
  return 0;  
}

int usqld_do_query(usqld_tc * tc, usqld_packet * packet){
  XDR_tree * reply = NULL;
  int rv;
  if(NULL==tc->db){
    reply = usqld_error_packet(USQLD_NOT_OPEN,
			       "You do not have a database open");
    goto query_send_reply;
  }
  
 query_send_reply:
  assert(reply!=NULL);
  rv =  usqld_send_packet(reply);
  XDR_tree_free(reply);
  return rv; 
  
}
void * usqld_conhandler_main(usqld_conhand_init  * init){
  usqld_tc tc;
  usqld_packet * p = NULL;
  int rv,resp_rv = USQLD_OK;
  tc.db =NULL;
  tc.client_fd = init->fd;
  tc.config = init->config;

  free(init);
  
  while(USQLD_OK==(rv=usqld_recv_packet(tc.client_fd,&p))){
    switch(usqld_get_packet_type(p)){
    case PICKLE_CONNECT:
      resp_rv = usqld_do_connect(&tc,p);
      break;
    case PICKLE_QUERY: 
      resp_rv = usqld_do_query(&tc,p);
      break;
    default:
      {
	usqld_packet * p;
	p = usqld_error_packet(USQLD_UNSUPPORTED,
			       "that command isn't supported yet");
	usqld_send_packet(tc.client_fd,p);
      }
    }
    if(resp_rv!=USQLD_OK){
      fprintf(stderr,"error %d while sending response\n",resp_rv);
      break;
    }
  }
  
  if(tc.db){
    sqlite_close(tc.db);
  }
  if(tc.database){
    free(tc.database);
  }

  return NULL;
}


  
 
