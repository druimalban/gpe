
#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "xdr.h"
#define USQLD_PROTOCOL_VERSION "0.2.0"

pthread_mutex_t protocol_schema_lock = PTHREAD_MUTEX_INITIALIZER;
XDR_schema * usqld_protocol_schema = NULL;

XDR_schema * usqld_make_protocol(){
  XDR_schema * protocol,
    *connect_packet,
    *disconnect_packet,
    *query_packet,
    *start_rows_packet,
    *rows_packet,
    *eof_packet,
    *connect_elems[2];
   XDR_union_discrim  elems[6];
   
   elems[0].t = eof_packet = XDR_schema_new_void();
   elems[0].d = PICKLE_EOF;
   elems[1].t = rows_packet = XDR_schema_new_array(XDR_schema_new_string(),5);
   elems[1].d = PICKLE_ROW;
   elems[2].t = start_rows_packet = XDR_schema_new_array(XDR_schema_new_string(),0);
   elems[2].d = PICKLE_STARTROWS;
   elems[3].t = query_packet = XDR_schema_new_string();
   elems[3].d = PICKLE_QUERY;
   elems[4].t = disconnect_packet = XDR_schema_new_void();
   elems[4].d = PICKLE_DISCONNECT;
   connect_elems[1] = connect_elems[0] = XDR_schema_string();
   
   elems[5].t  = connect_packet = XDR_schema_new_struct(2,connect_elems);
   elems[5].d = PICKLE_CONNECT; 
   protocol = XDR_schema_new_type_union(6,elems);   
   return protocol;
}

XDR_schema * usqld_get_protocol(){
   assert(0!=pthread_mutex_lock(&protocol_schema_lock));
   {  
     if(usqld_protocol_schema==NULL)
       usqld_protocol_schema=usqld_make_protocol();
   }
   assert(0!=pthread_mutex_unlock(&protocol_schema_lock));
   return usqld_protocol_schema;
}

int usqld_recv_packet(int fd,XDR_tree ** packet){
  return XDR_deserialize_elem(usqld_get_protocol(),fd,packet);
}

int usqld_send_packet(int fd,XDR_tree* packet){
  return XDR_serialize_elem(usqld_get_protocol(),packet,fd);
}

int usqld_get_packet_type(usqld_packet *packet){
  return XDR_t_get_union_disc(packet);
}
