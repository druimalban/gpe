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
#include <stdlib.h>
#include "usql.h"
#include "usqld-server.h"
#include "usqld-conhandler.h"
#include "usqld-protocol.h"
#include <errno.h>
#include <signal.h>
#define FILENAME_MAX 512

void sigpipe_handler(int sig){
  fprintf(stderr,"Broken pipe on FD \n");
}
/*
  configuration structure for a thread
 */
typedef struct{
  int client_fd;
  sqlite * db;
  char * database_name;
  usqld_config * config;
}usqld_tc;

/*
  checks Incoming stream to see if a packet header is there
  return the the start (i.e. the type) of the incomming packet
  used to deal with an interrupt send during a packet transmit
 */
unsigned int usqld_peek_next_packet(int fd){
  unsigned char header[4];
  
   if(4== recv(fd,header,4,MSG_DONTWAIT|MSG_PEEK)){
      return ntohl(*((unsigned int *)header));
   }
   return 0;
}

/*
  implements the connection request.
 */
int  usqld_do_connect(usqld_tc * tc,usqld_packet * p){
  char * database = NULL;
  char * version = NULL;
  XDR_tree_compound * content;
  usqld_packet * reply = NULL;
  int rv = USQLD_OK;
  char * errmsg = NULL;
  char fn_buf[FILENAME_MAX];

  if(tc->db!=NULL){
    reply = usqld_error_packet(SQLITE_CANTOPEN,"database already open");
    goto connect_send_reply;
  }

  content = XDR_TREE_COMPOUND(XDR_t_get_union_t(XDR_TREE_COMPOUND(p)));
  
  version = XDR_t_get_string(XDR_TREE_STR(XDR_t_get_comp_elem(content,0)));
  database = XDR_t_get_string(XDR_TREE_STR(XDR_t_get_comp_elem(content,1)));
  
  fprintf(stderr,"got connect (%s,%s)\n",version,database);
  
  if(strcmp(version,USQLD_PROTOCOL_VERSION)!=0){
    char buf[256];
    snprintf(buf,256,"Your client's protocol version (%s) does not match"\
	     " the server version (%s)",version,USQLD_PROTOCOL_VERSION);
    
    reply = usqld_error_packet(USQLD_VERSION_MISMATCH,buf);
    fprintf(stderr,"Version Mismatch\n");
    goto connect_send_reply;
  }

  strncpy(fn_buf,tc->config->db_base_dir,FILENAME_MAX);
  strncat(fn_buf,database,FILENAME_MAX);

  if(NULL==(tc->db=sqlite_open(fn_buf,0644,&errmsg))){
    reply = usqld_error_packet(SQLITE_CANTOPEN,errmsg);
    goto connect_send_reply;
  }
  sqlite_busy_timeout(tc->db,101);
  tc->database_name = strdup(database);
  /*we are set*/
  reply = usqld_ok_packet();
  goto connect_send_reply;
  
  connect_send_reply:
  assert(reply!=NULL);
  rv =  usqld_send_packet(tc->client_fd,reply);
  XDR_tree_free(reply);
  return rv; 
}

/*
  structure passed to the row-send
 
 */
typedef struct{
  usqld_tc * tc;
  int headsent;
  int rv;
  int terminate_now;
  int interrupted;
}usqld_row_context;

int usqld_send_row(usqld_row_context * rc,
		   int nfields,		   char ** fields,
		   char ** heads){
  
  XDR_tree *rowpacket = NULL;
  XDR_tree_compound *field_elems;
  int i;
  int rv;
//  fprintf(stderr,"about to try sending a row\n");  
  if(PICKLE_INTERRUPT==usqld_peek_next_packet(rc->tc->client_fd)){
    usqld_packet * p;
//    fprintf(stderr,"Interupted!\n");
    
    usqld_recv_packet(rc->tc->client_fd,&p);
    XDR_tree_free(p);
    p=XDR_tree_new_union(PICKLE_INTERRUPTED,XDR_tree_new_void());
    usqld_send_packet(rc->tc->client_fd,p);
    
    rc->interrupted = 1;//we have been sent an interrupt, give up now
    return -1;
  }
  
#ifdef VERBOSE_DEBUG
  for(i = 0;i<nfields;i++){
    fprintf(stderr,"\t%s,",heads[i]);
  }
  fprintf(stderr,"]\n");
  fprintf(stderr,"(");
  for(i = 0;i<nfields;i++){
    fprintf(stderr,"\t%s,",fields[i]);
     }
  fprintf(stderr,")\n");
#endif
     
  if(!rc->headsent){
    XDR_tree  * srpacket = NULL;
    XDR_tree_compound  *head_elems = NULL;
    
    XDR_tree_new_compound(XDR_VARARRAY,
			  nfields,
			  &head_elems);
    for( i =0;i<nfields;i++){
      XDR_t_set_comp_elem(head_elems,i,XDR_tree_new_string(heads[i]));
    }
    
    
    srpacket = XDR_tree_new_union(PICKLE_STARTROWS,
				  (XDR_tree*)head_elems);
    if(USQLD_OK!=(rv=usqld_send_packet(rc->tc->client_fd,
				       srpacket))){
      XDR_tree_free(srpacket);
      rc->rv = rv;
      rc->terminate_now =1;
      return 1;			 
    }
    XDR_tree_free(srpacket);
    rc->headsent = 1;
  }
  
  XDR_tree_new_compound(XDR_VARARRAY,
			nfields,
			(XDR_tree_compound**)&field_elems);
  
  for(i = 0;i<nfields;i++){
    XDR_t_set_comp_elem(field_elems,i,XDR_tree_new_string(fields[i]));
  }
  rowpacket = XDR_tree_new_union(PICKLE_ROW,
				 (XDR_tree*)field_elems);
  
  if(USQLD_OK!=(rv=usqld_send_packet(rc->tc->client_fd,
				     rowpacket))){
    XDR_tree_free(rowpacket);
    rc->rv = rv;
    return 1;
    
  }
  XDR_tree_free(rowpacket);
  return 0;  
}

/*
  implements a the response to a query request
 */
int usqld_do_query(usqld_tc * tc, usqld_packet * packet){
  XDR_tree * reply = NULL;
  int rv =0;
  char * sql,*errmsg;
  usqld_row_context rc;
  
  rc.tc = tc;
  rc.rv =0;
  rc.headsent = 0;
  rc.terminate_now = 0;
  rc.interrupted = 0;
  if(NULL==tc->db){
    reply = usqld_error_packet(USQLD_NOT_OPEN,
			       "You do not have a database open");
    goto query_send_reply;
  }
  sql = XDR_t_get_string(
          XDR_TREE_STR(
	    XDR_t_get_comp_elem(XDR_TREE_COMPOUND(packet),1)));


//  fprintf(stderr,"About to try and exec the sql: \"%s\"\n",sql);
 
  if(SQLITE_OK!=(rv=sqlite_exec(tc->db,
				sql,
				(sqlite_callback)usqld_send_row,
				(void*)&rc,
				&errmsg))){
    
    
    if(rc.interrupted){
      rv = 0;
      goto query_send_reply;// no reply no error
    }
    if(rc.terminate_now)
      goto query_send_reply; //fatal error
    if(errmsg==NULL){
      reply = usqld_error_packet(rv,"Unknown sql error");
    }else{
      reply = usqld_error_packet(rv,errmsg);
    }
    
    //fatal termination
    
    goto query_send_reply; 
  }else
    reply = usqld_ok_packet();
  
 query_send_reply:

  if(reply){
    rv =  usqld_send_packet(tc->client_fd,reply);
    XDR_tree_free(reply);
  }
  
  return rv; 
  
}
  
void * usqld_conhandler_main(usqld_conhand_init  * init){
  usqld_tc tc;
  usqld_packet * p = NULL;
  int rv = USQLD_OK,resp_rv = USQLD_OK;

  bzero(&tc,sizeof(usqld_tc));


  
   
  tc.db =NULL;
  tc.client_fd = init->fd;
  tc.config = init->config;
  
   
  free(init);
  
  while(USQLD_OK==(rv=usqld_recv_packet(tc.client_fd,&p))){
    switch(usqld_get_packet_type(p)){
    case PICKLE_INTERRUPT:
      {
	//we could still have a interrupt after we have finished
	//sending a query-result
	//so we send an INTERRUPTED packet anway
	XDR_tree  * out_p;
	out_p = XDR_tree_new_union(PICKLE_INTERRUPTED,
				   XDR_tree_new_void());
	resp_rv = usqld_send_packet(tc.client_fd,out_p);
	XDR_tree_free(out_p);
      }
    case PICKLE_CONNECT:
      resp_rv = usqld_do_connect(&tc,p);
      break;
    case PICKLE_QUERY: 
      resp_rv = usqld_do_query(&tc,p);
      break; 
    default:
      {
	fprintf(stderr,"unsupported packet type %d\n",
		usqld_get_packet_type(p));
	break;
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


  
 
