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
#include <assert.h>
#include "usqld-server.h"
#include "usqld-conhandler.h"
#include "usqld-protocol.h"

typedef struct{
  int client_fd;
  sqlite * db;
  char * database_name;
  usqld_config * config;
}usqld_tc;

int  usqld_do_connect(usqld_tc * tc,usqld_packet * p){
  char * database = NULL;
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
  version = XDR_t_get_string(XDR_t_get_comp_elem(content,0));
  database = XDR_t_get_string(XDR_t_get_comp_elem(content,1));
  fprintf(stderr,"got connect (%s,%s)\n",version,database);
  if(strcmp(version,USQLD_PROTOCOL_VERSION)!=0){
    char buf[256];
    snprintf(buf,256,"Your client's protocol version (%s) does not match"\
	     " the server version (%s)",version,USQLD_PROTOCOL_VERSION);
    
    reply = usqld_error_packet(USQLD_VERSION_MISMATCH,buf);
    goto connect_send_reply;
  }
  strncpy(fn_buf,tc->config->db_base_dir,512);
  strcat(fn_buf,database);
  
  if(NULL==(tc->db=sqlite_open(fn_buf,0644,&errmsg))){
    reply = usqld_error_packet(SQLITE_CANTOPEN,errmsg);
    goto connect_send_reply;
  }
  tc->database_name = strdup(database);
  /*we are set*/
  fprintf(stderr,"database:%s is now open,sending OK\n",database);
  reply = usqld_ok_packet();
  goto connect_send_reply;
 connect_send_reply:
  assert(reply!=NULL);
  rv =  usqld_send_packet(tc->client_fd,reply);
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
  int rv;
  fprintf(stderr,"about to send a row:\n[");
  for(i = 0;i<nfields;i++){
    fprintf(stderr,"\t%s,",heads[i]);
  }
  fprintf(stderr,"]\n");
  fprintf(stderr,"(");
  for(i = 0;i<nfields;i++){
    fprintf(stderr,"\t%s,",fields[i]);
  }
  fprintf(stderr,")\n");

  if(!rc->headsent){
    XDR_tree  * srpacket = NULL, *head_elems;

    XDR_tree_new_compound(XDR_VARARRAY,
			  nfields,
			  (XDR_tree_compound**)&head_elems);
    for( i =0;i<nfields;i++){
      XDR_t_get_comp_elem(head_elems,i) = XDR_tree_new_string(heads[i]);
    }
    
    
    srpacket = XDR_tree_new_union(PICKLE_STARTROWS,
				  head_elems);
    if(USQLD_OK!=(rv=usqld_send_packet(rc->tc->client_fd,
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
			(XDR_tree_compound**)&field_elems);
  
  for(i = 0;i<nfields;i++){
    XDR_t_get_comp_elem(field_elems,i) = XDR_tree_new_string(fields[i]);
  }
  rowpacket = XDR_tree_new_union(PICKLE_ROW,
				 field_elems);

  if(USQLD_OK!=(rv=usqld_send_packet(rc->tc->client_fd,
				     rowpacket))){
      XDR_tree_free(rowpacket);
      rc->rv = rv;
      return -1;			 
      
  }
  XDR_tree_free(rowpacket);
  return 0;  
}

int usqld_do_query(usqld_tc * tc, usqld_packet * packet){
  XDR_tree * reply = NULL;
  int rv;
  char * sql,*errmsg;
  usqld_row_context rc;
  
  rc.tc = tc;
  rc.rv =0;
  rc.headsent = 0;

  if(NULL==tc->db){
    reply = usqld_error_packet(USQLD_NOT_OPEN,
			       "You do not have a database open");
    goto query_send_reply;
  }
  sql = XDR_t_get_string(XDR_t_get_comp_elem(packet,1));
  fprintf(stderr,"About to try and exec the sql: \"%s\"\n",sql);
  
  if(SQLITE_OK!=(rv=sqlite_exec(tc->db,
				sql,
				(sqlite_callback)usqld_send_row,
				(void*)&rc,
				&errmsg))){
    fprintf(stderr,"error executing sql:%s\n",errmsg);
    if(errmsg==NULL){
      reply = usqld_error_packet(rv,"Unknown sql error");
    }else{
      reply = usqld_error_packet(rv,errmsg);
      free(errmsg);
    }
    
    goto query_send_reply; 
  }
  reply = usqld_ok_packet();
	        
 query_send_reply:
  assert(reply!=NULL);
  rv =  usqld_send_packet(tc->client_fd,reply);
  //XDR_tree_free(reply);
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
  if(rv!=USQLD_OK){
    fprintf(stderr,"error %d while demarshalling request\n",rv);
  }
  

  close(tc.client_fd);
  if(tc.db){
    sqlite_close(tc.db);
  }
  if(tc.database_name){
    free(tc.database_name);
  }

  return NULL;
}


  
 
