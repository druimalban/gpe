#include <stdio.h>
#include "xdr.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h> 
#include <sys/fcntl.h>

#define PICKLE_CONNECT 0x0
#define PICKLE_DISCONNECT 0x1
#define PICKLE_QUERY 0x2
#define PICKLE_ERROR 0x3
#define PICKLE_OK 0x4
#define PICKLE_STARTROWS 0x5
#define PICKLE_ROW 0x6
#define PICKLE_EOF 0x7
#define PICKLE_MAX 0x8

int main(int argc,char ** argv){
  
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
  
  
  XDR_dump_schema(protocol);

  fd = open("test",O_RDONLY);
  
  
  
}

