
#include <xdr.h>
#include "example-schema.h"

XDR_typedesc * get_schema(){
  
  XDR_typedesc * protocol,
    *connect_packet,
    *disconnect_packet,
    *query_packet,
    *start_rows_packet,
    *rows_packet,
    *eof_packet;
 
  XDR_union_discrim  elems[6];
  XDR_tree * dom =NULL;
  int fd;

  
  elems[0].t = eof_packet = XDR_schema_new_void();
  elems[0].d = PICKLE_EOF;
  elems[1].t = rows_packet = XDR_schema_new_array(XDR_schema_new_string(),0);
  elems[1].d = PICKLE_ROW;
  elems[2].t = start_rows_packet = XDR_schema_new_array(XDR_schema_new_string(),0);
  elems[2].d = PICKLE_STARTROWS;
  elems[3].t = query_packet = XDR_schema_new_string();
  elems[3].d = PICKLE_QUERY;
  elems[4].t = disconnect_packet = XDR_schema_new_void();
  elems[4].d = PICKLE_DISCONNECT;
  elems[5].t  = connect_packet = XDR_schema_new_string();
  elems[5].d = PICKLE_CONNECT; 
  
  protocol = XDR_schema_new_type_union(6,elems);
  
  return protocol;
}

