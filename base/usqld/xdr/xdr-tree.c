#include "xdr.h"
#include <sys/types.h>
#include <stdlib.h>
#include "xdr.h"
#include <assert.h>

int  XDR_tree_new_compound(XDR_type type,
			   size_t nelems,
			   XDR_tree_compound ** out_t){
  XDR_tree_compound * t ;
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
  bzero(t->val,sizeof(XDR_tree_simple_val));
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
  if(NULL !=t->data){
    free(t);
    return XDR_PMALLOC;
  }
  bzero(t->data,len);

  *out_t = t;
  return XDR_OK;
}


void XDR_tree_free(XDR_tree * t){

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
  XDR_compound * t = NULL; 
  
  
}

XDR_tree* XDR_tree_new_struct (size_t num_elems,
			       XDR_tree ** elems);

XDR_tree * XDR_tree_new_array(int fixed_size,
			      size_t num_elems,
			      XDR_tree **elems);

XDR_tree * XDR_tree_new_compound(int fixed_size,
				 size_t length,
				 unsigned char * data);

XDR_tree * XDR_tree_new_string(size_t length,
			       unsigned char * data);

XDR_tree * XDR_tree_new_num(XDR_type t,u_int32_t val);

XDR_tree * XDR_tree_new_uhyper(u_int64_t val);

XDR_tree * XDR_tree_new_float(float val);
XDR_tree * XDR_tree_new_double(double val);
