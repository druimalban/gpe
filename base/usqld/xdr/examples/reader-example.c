#include <stdlib.h>
#include <stdio.h>
#include <xdr.h>
#include <assert.h>
#include "example-schema.h"


int main(int argc, char ** argv){
  XDR_schema * schema;
  XDR_tree * data;
  int i;
  int rv;

  schema = get_schema();

  if(XDR_OK!=(rv=XDR_deserialize_elem(schema,fileno(stdin),&data))){
    fprintf(stderr,"Error deserializing output: %d\n",rv);
  }else{
    XDR_tree_dump(data);
  }
  


  
  XDR_tree_free(data);
}
