#include <sys/types.h>
#include <stdlib.h>
#include "xdr.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int  XDR_tree_new_compound(XDR_type type,
			   size_t nelems,
			   XDR_tree_compound ** out_t){
  XDR_tree_compound * t;
  assert(NULL!=out_t);

  t = XDR_malloc(XDR_tree_compound);
  assert(NULL!=t);
  t->type = type;
  t->subelems = XDR_mallocn(XDR_tree*,nelems);

  
  if(NULL==t->subelems){
    XDR_free(t);
    return XDR_PMALLOC;
  }
  bzero(t->subelems,sizeof(XDR_tree*) * nelems);
  t->nelems = nelems;
  assert(t->subelems!=NULL);
  *out_t   = t;
  return XDR_OK;
}


void XDR_tree_new_simple(XDR_type type,XDR_tree_simple **out_t){
  XDR_tree_simple * t = XDR_malloc(XDR_tree_simple);
  assert(NULL!=t);
  t->type = type;
  bzero(&(t->val),sizeof(XDR_tree_simple_val));
  *out_t = t;
}

int  XDR_tree_new_str (XDR_type type,
		       size_t len,
		       XDR_tree_str** out_t){
  
  XDR_tree_str * t = XDR_malloc(XDR_tree_str);
  assert(NULL!=t);
  t->type = type;
  t->len = len;

  if(type==XDR_STRING)
    len++; //null char;

  t->data = XDR_mallocn(unsigned char,len);
  if(NULL==t->data){
    XDR_free(t);
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
	XDR_free(o->data);    
    }
    break;
  case XDR_STRUCT:
  case XDR_FIXEDARRAY:
  case XDR_VARARRAY:
  case XDR_UNION:
    {
      int i;
      XDR_tree_compound * c = (XDR_tree_compound *)t;
      
      for(i=0;i<c->nelems;i++){
	if(NULL!=c->subelems[i])
	  XDR_tree_free(c->subelems[i]);
      }
      
      if(c->subelems!=NULL){
	XDR_free(c->subelems);
      }
    }
    break;
  default:
    ;
    
  }
  
  XDR_free(t);
}


XDR_tree*  XDR_tree_new_union (unsigned int disc,
			       XDR_tree * val){
  XDR_tree_compound * t = NULL; 

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
			       size_t length,
			       unsigned char * data){
  XDR_tree_str *t;
  if(XDR_OK !=XDR_tree_new_str(type,length,&t))
    return NULL;
  memcpy(t->data,data,length);
  return (XDR_tree*)t;
}


XDR_tree * XDR_tree_new_string(const char * data){
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

   
   printf("%s:",t==NULL?"NULL":XDR_find_type_name(t->type));
   if(t==NULL)
     return;
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
    printf("%f",((XDR_tree_simple*)t)->val.floatVal);
     break;
  case XDR_FIXEDOPAQUE:
  case XDR_VAROPAQUE:
    printf("[%u]",((XDR_tree_str*)t)->len);
    break;
  case XDR_STRING:
    printf("\"%s\"",((XDR_tree_str*)t)->data);;
    break;
  case XDR_VARARRAY:
  case XDR_STRUCT:
  case XDR_FIXEDARRAY:
    {
      int i;
      printf("[%d]{\n",((XDR_tree_compound*)t)->nelems);
      do_tabs(indent+1);
      for(i = 0;i<((XDR_tree_compound*)t)->nelems;i++){
	do_tabs(indent);
	XDR_tree_dump_r(((XDR_tree_compound*)t)->subelems[i],indent+1);
      }
      printf("}");
      break;
    }

  case XDR_UNION:
    {
       printf("(%d)=>",((XDR_tree_simple*)
			((XDR_tree_compound*)t)->subelems[0])->val.uintVal);
       XDR_tree_dump_r(((XDR_tree_compound*)t)->subelems[1],indent+1);
       
    }
  break;
  default:    
    ;
  }
   printf("\n");
}

void XDR_tree_dump(XDR_tree*t){
  XDR_tree_dump_r(t,0);
}

XDR_tree * XDR_tree_new_void(){
  XDR_tree * tree;
  tree = XDR_malloc(XDR_tree);
  bzero(tree,sizeof(XDR_tree));
  tree->type = XDR_VOID;
  return tree;
}


int XDR_tree_is_simple(XDR_tree * t){
  assert(t!=NULL);
  return(t->type==XDR_INT||
	 t->type==XDR_UINT||
	 t->type==XDR_ENUM||
	 t->type==XDR_BOOL||
	 t->type==XDR_FLOAT||
	 t->type==XDR_DOUBLE);
	
}

int XDR_tree_is_compound(XDR_tree * t){
  assert(t!=NULL);
  return (t->type==XDR_STRUCT || 
	  t->type==XDR_FIXEDARRAY ||
	  t->type==XDR_VARARRAY || 
	  t->type==XDR_UNION);
}

int XDR_tree_is_str(XDR_tree * t){
  assert(t!=NULL);
  return(t->type==XDR_STRING||
	 t->type==XDR_VAROPAQUE||
	 t->type==XDR_FIXEDOPAQUE);
  
}
/**
   retrieves an arbitray compound element from a compound. 
 */
XDR_tree *  XDR_t_get_comp_elem(XDR_tree_compound * t,unsigned int  n){
  assert(t!=NULL);
  assert(n <t->nelems);
  return t->subelems[n];
}

/**
   inserts an arbitrary (optionally NULL XDR_tree into a compound) 
   Does not do schema checking. 
 */
void  XDR_t_set_comp_elem(XDR_tree_compound * t, 
			  unsigned int n, XDR_tree * val){
  assert(t!=NULL);
  assert(n <t->nelems);
  t->subelems[n] =val;
}
unsigned int XDR_t_get_comp_len(XDR_tree_compound * t){
  assert(t!=NULL);
  return t->nelems;
}

unsigned int XDR_t_get_union_disc(XDR_tree_compound * t){
  assert(t!=NULL);
  assert(t->type ==XDR_UNION);
  assert(t->nelems ==2);
  assert(t->subelems[0] !=NULL);
  assert(t->subelems[0]->type==XDR_UINT);
  
  return ((XDR_tree_simple*) t->subelems[0])->val.uintVal;
  
}

XDR_tree * XDR_t_get_union_t(XDR_tree_compound * t){
  assert(t!=NULL);
  assert(t->type ==XDR_UNION);
  assert(t->nelems ==2);
  return t->subelems[1];
}

char *  XDR_t_get_string(XDR_tree_str * t){
  assert(t!=NULL);
  assert(t->type ==XDR_STRING);
  return t->data;
}

unsigned char *  XDR_t_get_data(XDR_tree_str * t){
  assert(t!=NULL);
  assert(t->type ==XDR_FIXEDOPAQUE || 
	 t->type== XDR_VAROPAQUE);
  return t->data;
}

size_t XDR_t_get_data_len(XDR_tree_str * t){
  assert(t!=NULL);
  assert(t->type ==XDR_FIXEDOPAQUE || 
	 t->type== XDR_VAROPAQUE);
  return t->len;
}

unsigned int XDR_t_get_uint(XDR_tree_simple * t){
  assert(t->type==XDR_UINT || 
	 t->type==XDR_INT || 
	 t->type==XDR_ENUM );
  return t->val.uintVal;
}

     
