#include <stdlib.h>
#include <stdio.h>
#include <xdr.h>
#include <assert.h>
#include "example-schema.h"


int main(int argc, char ** argv){
  XDR_schema * schema;
  XDR_tree * data;
  char * row[] ={"fish","foo","bar","wibble"};
  XDR_tree *trow[4];
  int i;
  int rv;

  schema = get_schema();

  for(i = 0;i<4;i++){
    trow[i]=XDR_tree_new_string(row[i]);
    assert(trow[i]!=NULL);
  }
  data = XDR_tree_new_union(PICKLE_ROW,
			    XDR_tree_new_var_array(4,trow));
  
  assert(data!=NULL);

  if(XDR_OK!=(rv=XDR_serialize_elem(schema,data,fileno(stdout)))){
    fprintf(stderr,"Error serializing output: %d\n",rv);
  }
  
  
  XDR_tree_free(data);
}
