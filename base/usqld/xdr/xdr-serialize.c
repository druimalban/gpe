#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>

#include "xdr.h"

int  XDR_serialize_elem(XDR_typedesc * s, XDR_tree *t,int fd){
  unsigned char buf[4];
  int rv = XDR_OK;

  if(s->type!=t->type){
    return XDR_SCHEMA_VIOLATION;
  }
  switch(s->type){
  case XDR_INT:
  case XDR_BOOL:
  case XDR_UINT:
    {
      *((u_int32_t*)buf) = htonl(((XDR_tree_simple*)t)->val.uintVal);
      if(-1==write(fd,buf,4))
	return XDR_IO_ERROR;

    };break;
  case XDR_FLOAT: 
    {
      if(-1==write(fd,&(((XDR_tree_simple*)t)->val.floatVal),4))
	return XDR_IO_ERROR;
    };break;
  case XDR_HYPER:
  case XDR_UHYPER:
  case XDR_DOUBLE:
  case XDR_LONGDOUBLE:
    return XDR_UNSUPPORTED;
  case XDR_FIXEDOPAQUE:
  case XDR_VAROPAQUE:   
  case XDR_STRING:
    {
      size_t slen;
      unsigned char * data;
      
      data = ((XDR_tree_str*)t)->data;
      slen = ((XDR_tree_str*)t)->len;;
      
      *buf = htonl(slen);
      if(-1==write(fd,buf,4))
	 return XDR_IO_ERROR;
	 
      while(slen){
	if(slen < 4){
	  memcpy(buf,data,slen);
	  bzero(buf + slen,4-slen);
	  slen -=slen;
	}else{
	  memcpy(buf,data,4);
	  slen-=4;
	  data +=4;
	}	
	if(-1==write(fd,buf,4))
	  return XDR_IO_ERROR;
       
      }
    }
    break;
  case XDR_FIXEDARRAY: 
  case XDR_VARARRAY:
    {
      size_t len;
      int i;
      
      if(s->type==XDR_FIXEDARRAY&& 
	 ((XDR_array*)s)->num_elems!=((XDR_tree_compound*)t)->nelems){
	return XDR_SCHEMA_VIOLATION;
      }

      len = ((XDR_tree_compound*)t)->nelems;
      *((u_int32_t*)buf) = htonl(len);
      if(-1==write(fd,buf,4))
	return XDR_IO_ERROR;
      
      for(i =0;i<len;i++){
	if(XDR_OK!=(rv =XDR_serialize_elem(((XDR_array*)s)->elem_type,
					   ((XDR_tree_compound*)t)->subelems[i],fd)))
	  return rv;
	
      }
    }
    break;
  case XDR_STRUCT:
    {
      size_t nelems;
      int i;
      if((nelems= ((XDR_struct *)t)->num_elems)!=
	 ((XDR_tree_compound *)s)->nelems)
	return XDR_SCHEMA_VIOLATION;
      
      for(i = 0;i<nelems;i++){
	if(XDR_OK!=(rv=XDR_serialize_elem(((XDR_struct *)s)->elems[i],
					  ((XDR_tree_compound*)t)->subelems[i],fd)))
	  return rv;
      }
      
    }
    break;
  case XDR_UNION:
    {
      int i;
      unsigned int d_val;
      XDR_typedesc * d_t = NULL;
      
      if(((XDR_tree_compound*)s)->nelems!=2 ||
	 ((XDR_tree_compound*)s)->subelems[0]->type!=XDR_UINT){
	return XDR_SCHEMA_VIOLATION;
      }
      d_val = ((XDR_tree_simple*)(((XDR_tree_compound*)t)->subelems[0]))->val.uintVal;
      
      for(i=0;i<((XDR_type_union*)s)->num_alternatives;i++){	
	if(((XDR_type_union*)s)->elems[i].d==d_val){
	  d_t = ((XDR_type_union*)s)->elems[i].t;
	  break;
	}
      }
      
      if(NULL==d_t || 
	 ((XDR_tree_compound*)t)->subelems[1]->type!=d_t->type)
	{
	  return XDR_SCHEMA_VIOLATION;
	}
      
      *((int*)buf) = htonl(d_val);
      if(-1==write(fd,buf,4))
	return XDR_IO_ERROR;
      
      if(XDR_OK!=(rv=XDR_serialize_elem(d_t,((XDR_tree_compound*)t)->subelems[1],
					fd)))
	return rv;     
    }
    break;
  case XDR_VOID:
    break;
  defauilt:
    return XDR_SCHEMA_VIOLATION;
  }
  
  return XDR_OK;
}
