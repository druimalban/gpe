#include <sys/types.h>
#include <stdlib.h>
#include "xdr.h"
#include <assert.h>
/*
  constructor for all the basic types and void
 */
XDR_schema * XDR_schema_new_typedesc(XDR_type t){
  XDR_schema * r;
  r = mylloc(XDR_schema);
  assert(NULL!=r);
  r->type = t;
  return r;
}



XDR_schema * XDR_schema_new_opaque(size_t len){
  XDR_opaque * r;
  r = mylloc(XDR_opaque);
  assert(NULL!=r);
  r->num_elems = len;
  if(0==len)
    r->base.type = XDR_VAROPAQUE;
  else
    r->base.type = XDR_FIXEDOPAQUE;
  return (XDR_schema *) r;
}

XDR_schema * XDR_schema_new_array(XDR_schema * t,size_t len){
  XDR_array * r;
  r = mylloc(XDR_array);
  assert(NULL!=r);
  r->num_elems = len;
  if(len==0)
    r->base.type = XDR_VARARRAY;
  else
    r->base.type = XDR_FIXEDARRAY;
  
  r->elem_type = t;

  return (XDR_schema*) r;
}

XDR_schema * XDR_schema_new_struct(size_t num_elems,const XDR_schema ** elems){
  XDR_struct * r;
  
  r = mylloc( XDR_struct);
  assert(r!=NULL);
  r->base.type = XDR_STRUCT;
  r->elems = myllocn(XDR_schema*,num_elems);
  assert(r->elems!=NULL);
  memcpy(r->elems,elems,sizeof(XDR_schema *) * num_elems);
  r->num_elems = num_elems;
  return (XDR_schema*) r;
}


XDR_schema * XDR_schema_new_type_union(size_t num_alternatives,
					 const XDR_union_discrim *elems){
  XDR_type_union *  r;

  r = mylloc(XDR_type_union);
  assert(NULL!=r);
  r->elems = myllocn(XDR_union_discrim,num_alternatives);
  assert(NULL!=r->elems);
  r->num_alternatives = num_alternatives;
  memcpy(r->elems,elems,sizeof(XDR_type_union) * num_alternatives);
  r->base.type = XDR_UNION;
  return (XDR_schema *) r;
}

struct {
  XDR_type t;
  char * name;
} dump_table[] = 
{
  {XDR_INT,"INT"},
  {XDR_UINT,"UINT"},
  {XDR_ENUM,"UINT"},
  {XDR_BOOL,"BOOL"},
  {XDR_HYPER,"HYPER"},
  {XDR_UHYPER,"UHYPER"},
  {XDR_FLOAT,"FLOAT"},
  {XDR_DOUBLE,"DOUBLE"},
  {XDR_LONGDOUBLE,"LONGDOUBLE"},
  {XDR_STRING,"STRING"},
  {XDR_FIXEDOPAQUE,"FIXEDOPAQUE"},
  {XDR_VAROPAQUE,"VAROPAQUE"},
  {XDR_VARARRAY,"VARARRAY"},
  {XDR_STRUCT,"STRUCT"},
  {XDR_UNION,"UNION"},
  {XDR_VOID,"VOID"},
  {0,NULL}
};



const char * XDR_find_type_name(XDR_type t){
  int i;
  for(i = 0;dump_table[i].name;i++){
    if(t==dump_table[i].t)
      return dump_table[i].name;
  }
  return NULL;
    
}

void do_tab(int indent){
  while(indent--)
    printf("\t");
}
void XDR_dump_schema_r(XDR_schema *elem,int indent){
  switch(elem->type){
  case XDR_INT:
  case XDR_UINT: 
  case XDR_ENUM: 
  case XDR_BOOL:
  case XDR_HYPER:
  case XDR_UHYPER:
  case XDR_DOUBLE:
  case XDR_FLOAT: 
  case XDR_LONGDOUBLE:
  case XDR_VAROPAQUE:   
  case XDR_STRING:
  case XDR_VOID:
    printf("%s;\n",XDR_find_type_name(elem->type));
    break;
  case XDR_FIXEDOPAQUE:
    printf("%s[%d]\n",XDR_find_type_name(elem->type),((XDR_opaque*)elem)->num_elems);
    break;
  case XDR_FIXEDARRAY:
    printf("%s[%d]{\n",XDR_find_type_name(elem->type),((XDR_array*)elem)->num_elems);
    do_tab(indent+1);
    XDR_dump_schema_r(((XDR_array*)elem)->elem_type,indent+1);
    do_tab(indent);
    printf("}\n");
    break;
  case XDR_VARARRAY:
    printf("%s[*]{\n",XDR_find_type_name(elem->type));
    do_tab(indent+1);
    XDR_dump_schema_r(((XDR_array*)elem)->elem_type,indent+1);
    do_tab(indent);
    printf("}\n");
    break;
  case XDR_STRUCT:
    {
      int i;
      printf("%s[*]{\n",XDR_find_type_name(elem->type));
      for(i =0;i<((XDR_struct*)elem)->num_elems;i++){
	do_tab(indent+1);
	XDR_dump_schema_r(((XDR_struct*)elem)->elems[i],indent+1);
      }
      do_tab(indent);
      printf("}\n");
    }
    break;
  case XDR_UNION:
    {
      int i;
      
      printf("%s(%d){\n",XDR_find_type_name(elem->type),((XDR_type_union*)elem)->num_alternatives);
      for(i =0;i<((XDR_type_union*)elem)->num_alternatives;i++){
	do_tab(indent+1);
	printf("%d->",((XDR_type_union*)elem)->elems[i].d);
	XDR_dump_schema_r(((XDR_type_union*)elem)->elems[i].t,indent+1);
      }
      do_tab(indent);
      printf("}\n");
    }

  default:
    
  }
}
  

void XDR_dump_schema( XDR_schema *elem){
  XDR_dump_schema_r(elem,0);
}






