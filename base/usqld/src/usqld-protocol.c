
#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include "xdr.h"
#include "usqld-protocol.h"
char * USQLD_PROTOCOL_VERSION = "USQLD_0.3.0";
char * USQLD_VERSION = "USQLD_0.3.0";

struct {
  char * name;
  unsigned int p_type;
} protocol_elem_names[] = {
  {"PICKLE_CONNECT", PICKLE_CONNECT},
  {"PICKLE_DISCONNECT", PICKLE_DISCONNECT},
  {"PICKLE_QUERY", PICKLE_QUERY},
  {"PICKLE_ERROR", PICKLE_ERROR},
  {"PICKLE_OK", PICKLE_OK },
  {"PICKLE_STARTROWS", PICKLE_STARTROWS},
  {"PICKLE_ROW", PICKLE_ROW},
  {"PICKLE_EOF", PICKLE_EOF},
  {"PICKLE_MAX", PICKLE_MAX},
  {"PICKLE_INTERRUPT", PICKLE_INTERRUPT},
  {"PICKLE_INTERRUPTED", PICKLE_INTERRUPTED},
  {"PICKLE_REQUEST_ROWID", PICKLE_ROWID},
  {"PICKLE_ROWID", PICKLE_ROWID},
  {NULL,0x0}
};


char * usqld_find_packet_name(int packet_type){
  int i; 
  for(i= 0;protocol_elem_names[i].name!=NULL;i++){
    if(protocol_elem_names[i].p_type==packet_type)
      return protocol_elem_names[i].name;
  }
  return NULL;
}


/**
   the one and only instance of the usqld protocol
 */
XDR_schema * usqld_protocol_schema = NULL;

/**
   creates a new instance of the usqld protcol
 */
void  usqld_init_protocol(){
  XDR_schema 
    *connect_packet,
    *disconnect_packet,
    *query_packet,
    *start_rows_packet,
    *rows_packet,
    *eof_packet,
    *error_packet,
    *rowid_packet;
  
  XDR_schema * connect_elems[2];
  XDR_schema * err_elems[2];
  XDR_schema * top_elems[2];

  XDR_union_discrim  elems[12];
  
  elems[0].t = eof_packet = XDR_schema_new_void();
  elems[0].d = PICKLE_EOF;

  elems[1].t = rows_packet = XDR_schema_new_array(XDR_schema_new_string(),0);
  elems[1].d = PICKLE_ROW;
  
  elems[2].t = start_rows_packet = 
    XDR_schema_new_array(XDR_schema_new_string(),0);
  elems[2].d = PICKLE_STARTROWS;

  elems[3].t = query_packet = XDR_schema_new_string();
  elems[3].d = PICKLE_QUERY;
  
  elems[4].t = disconnect_packet = XDR_schema_new_void();
  elems[4].d = PICKLE_DISCONNECT;

  connect_elems[1] = connect_elems[0] = XDR_schema_new_string();  
  elems[5].t  = connect_packet = XDR_schema_new_struct(2,connect_elems);
  elems[5].d = PICKLE_CONNECT; 
  
  err_elems[0] = XDR_schema_new_uint();
  err_elems[1] = XDR_schema_new_string();
  elems[6].t = error_packet = XDR_schema_new_struct(2,err_elems);
  elems[6].d =PICKLE_ERROR;

  elems[7].t = XDR_schema_new_void();
  elems[7].d = PICKLE_OK;
  
  elems[8].t = XDR_schema_new_void();
  elems[8].d = PICKLE_INTERRUPT;
  
  elems[9].t = XDR_schema_new_void();
  elems[9].d = PICKLE_INTERRUPTED;
  
  elems[10].t = rowid_packet = XDR_schema_new_void();
  elems[10].d = PICKLE_REQUEST_ROWID;
  
  elems[11].t = rowid_packet = XDR_schema_new_uint();
  elems[11].d = PICKLE_ROWID;
   
  top_elems[0] = XDR_schema_new_uint();
  top_elems[1] =XDR_schema_new_type_union(12,elems);
  
  usqld_protocol_schema = XDR_schema_new_struct(2,top_elems);
   
}

pthread_once_t init_protocol_once = PTHREAD_ONCE_INIT;


/**
   returns an instance of the usqld protocol
   we only bother to instantiate the protocol once per session
   as it is read-only and can never be freed anyway.
   this does rather assume pthreads...
*/
XDR_schema * usqld_get_protocol(){
  
  pthread_once(&init_protocol_once,
	       usqld_init_protocol);
  
  
  
  return usqld_protocol_schema;
}

/**
   constructs an error packet. 
 */
usqld_packet * usqld_error_packet(int errcode, const char * str){
  XDR_tree * p;
  XDR_tree * ep[2];
  ep[0] =XDR_tree_new_uint(errcode);
  ep[1] = XDR_tree_new_string(str);

  p = XDR_tree_new_union(PICKLE_ERROR,
			 XDR_tree_new_struct(2,ep));
  return p;
}


/**
   constructs an acknowledgement packet packet
 */
usqld_packet * usqld_ok_packet(){
  XDR_tree * p;
  p = XDR_tree_new_union(PICKLE_OK,XDR_tree_new_void());
  return p;
}

/**
   constructs a rowid packet
 */
usqld_packet * usqld_rowid_packet(int rowid){
  XDR_tree * p;
	p = XDR_tree_new_union(PICKLE_ROWID, XDR_tree_new_uint(rowid));
  return p;
}

usqld_packet * usqld_env_get_packet(usqld_envelope * env){
  assert(NULL!=env);
  return XDR_t_get_comp_elem(XDR_TREE_COMPOUND(env),1);
}
