#include <sys/types.h>
#include <stdlib.h>
#include "xdr.h"
#include <assert.h>
#include <stdio.h>
int  XDR_tree_new_compound(XDR_type type,
			   size_t nelems,
			   XDR_tree_compound ** out_t){
  XDR_tree_compound * t;
  assert(NULL!=out_t);

  t = mylloc(XDR_tree_compound);
  assert(NULL!=t);
  t->type = type;
  t->subelems = myllocn(XDR_tree*,nelems);
  if(NULL==t->subelems){
    free(t);
    return XDR_PMALLOC;
  }
  bzero(t->subelems,sizeof(XDR_tree*) * nelems);
  t->nelems = nelems;
  *out_t   = t;
  return XDR_OK;
}


void XDR_tree_new_simple(XDR_type type,XDR_tree_simple **out_t){
  XDR_tree_simple * t = mylloc(XDR_tree_simple);
  assert(NULL!=t);
  t->type = type;
  bzero(&(t->val),sizeof(XDR_tree_simple_val));
  *out_t = t;
}

int  XDR_tree_new_str (XDR_type type,
		       size_t len,
		       XDR_tree_str** out_t){
  
  XDR_tree_str * t = mylloc(XDR_tree_str);
  assert(NULL!=t);
  t->type = type;
  t->len = len;

  if(type==XDR_STRING)
    len++; //null char;

  t->data = myllocn(unsigned char,len);
  if(NULL==t->data){
    free(t);
    return XDR_PMALLOC;
  }
  bzero(t->data,len);

  *out_t = t;
  return XDR_OK;
}


void XDR_tree_free(XDR_tree * t){

  if(t==NULL)
    return;
  switch(t->type){
  case XDR_STRING:
  case XDR_VAROPAQUE:
  case XDR_FIXEDOPAQUE:
    {
      XDR_tree_str * o = (XDR_tree_str*) t;
      if(NULL!=o->data)
	free(o->data);    
    }
    break;
  case XDR_STRUCT:
  case XDR_FIXEDARRAY:
  case XDR_VARARRAY:
  case XDR_UNION:
    {
      XDR_tree_compound * c = (XDR_tree_compound *)t;
      int i;
      
      for(i==0;i<c->nelems;i++){
	if(NULL!=c->subelems[i])
	  XDR_tree_free(c->subelems[i]);
      }
      
      if(c->subelems!=NULL){
	free(c->subelems);
      }
    }
  }
  
  free(t);
}


XDR_tree*  XDR_tree_new_union (unsigned int disc,
			       XDR_tree * val){
  XDR_tree_compound * t = NULL; 
  XDR_tree_simple * s = NULL;
  if(XDR_OK!=XDR_tree_new_compound(XDR_UNION,2,&t)){
    return NULL;
  }
  t->subelems[0] = XDR_tree_new_uint(disc);
  t->subelems[1] = val;
  
  return (XDR_tree *)t;
}

XDR_tree* XDR_tree_new_mult (XDR_type type,
			     size_t num_elems,
			     XDR_tree ** elems){
  XDR_tree_compound * t;
  
  if(XDR_OK!=XDR_tree_new_compound(type,
				   num_elems,
				   &t)){
    return NULL;
  }
  
  memcpy(t->subelems,elems,sizeof(XDR_tree*)* num_elems);
  return (XDR_tree *)t;
}


XDR_tree * XDR_tree_new_opaque(XDR_type type,
			       int fixed_size,
			       size_t length,
			       unsigned char * data){
  XDR_tree_str *t;
  if(XDR_OK !=XDR_tree_new_str(type,length,&t))
    return NULL;
  memcpy(t->data,data,length);
  return (XDR_tree*)t;
}


XDR_tree * XDR_tree_new_string(char * data){
  XDR_tree_str  * t = NULL;
  
  if(XDR_OK!=XDR_tree_new_str(XDR_STRING,strlen(data),&t))
    return NULL;
  
  strcpy(t->data,data);
  return (XDR_tree *)t;
}

XDR_tree * XDR_tree_new_num(XDR_type type,u_int32_t val){
  XDR_tree_simple *t  = NULL;
  XDR_tree_new_simple(type,&t);
  t->val.uintVal = val;
  return (XDR_tree*) t;
}

XDR_tree * XDR_tree_new_xhyper(XDR_type type,u_int64_t val){
  XDR_tree_simple *t = NULL;
  XDR_tree_new_simple(type,&t);
  t->val.uhypVal = val;
  return (XDR_tree*) t;  
}

XDR_tree * XDR_tree_new_float(float val){
  XDR_tree_simple *t = NULL;
  XDR_tree_new_simple(XDR_FLOAT,&t);
  t->val.floatVal = val;
  return (XDR_tree*) t;  
}
XDR_tree * XDR_tree_new_double(double val){
  XDR_tree_simple *t = NULL;
  XDR_tree_new_simple(XDR_DOUBLE,&t);
  t->val.doubleVal = val;
  return (XDR_tree*) t;  
}

void do_tabs(int n){
  while(n--){
    fprintf(stdout,"\t");
  }
}

void XDR_tree_dump_r(XDR_tree*t,int indent){
  printf("%s:",XDR_find_type_name(t->type));
  
  switch(t->type){
  case XDR_HYPER:
  case XDR_INT:
    printf("%d",((XDR_tree_simple*)t)->val.intVal);
    break;
  case XDR_UHYPER:
  case XDR_ENUM: 
  case XDR_BOOL:
  case XDR_UINT:
    printf("%u",((XDR_tree_simple*)t)->val.uintVal);
    break;
  case XDR_DOUBLE:
  case XDR_FLOAT: 
    printf("%f",((XDR_tree_simple*)t)->val.uintVal);
  case XDR_FIXEDOPAQUE:
  case XDR_VAROPAQUE:
    printf("[%u]",((XDR_tree_str*)t)->len);
  case XDR_STRING:
    printf("\"%s\"",((XDR_tree_str*)t)->data);;
    break;
  case XDR_VARARRAY:
  case XDR_STRUCT:
  case XDR_FIXEDARRAY:
    {
      int i;
      printf("[%d]{\n",((XDR_tree_compound*)t)->nelems);
      do_tab(indent+1);
      for(i = 0;i<((XDR_tree_compound*)t)->nelems;i++){
	do_tab(indent);
	XDR_tree_dump_r(((XDR_tree_compound*)t)->subelems[i],indent+1);
      }
      printf("}");
      break;
    }

  case XDR_UNION:
    {
      int i;
      
      printf("(%d)=>",((XDR_tree_simple*)
		       ((XDR_tree_compound*)t)->subelems[0])->val.uintVal);
      XDR_tree_dump_r(((XDR_tree_compound*)t)->subelems[1],indent+1);
      
    }
  default:    
  }
  printf("\n");
}

void XDR_tree_dump(XDR_tree*t){
  XDR_tree_dump_r(t,0);
}

XDR_tree * XDR_tree_new_void(){
  XDR_tree * tree;
  tree = mylloc(XDR_tree);
  bzero(tree,sizeof(XDR_tree));
  tree->type = XDR_VOID;
  return tree;
}
