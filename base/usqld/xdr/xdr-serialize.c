#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h> 
#include <netinet/in.h>
#include <string.h>
#include "xdr.h"
int debug_fd = 0;

int  XDR_serialize_elem(XDR_schema * in_s, 
			XDR_tree *in_t,int fd){

  unsigned char buf[4];
  int rv = XDR_OK;
  char * reason;
  char errbuf[1024];
  
#ifdef VERBOSE_DEBUG
  fprintf(stderr,
	  "About to serialize a %s using a %s\n",
	  XDR_find_type_name(in_s->type),
	  (in_t==NULL?"NULL":XDR_find_type_name(in_s->type)));
#endif
  
  if(in_s->type!=in_t->type){
#ifdef VERBOSE_DEBUG
    fprintf(stderr,"errrr, schema says %s, but type says %s\n",
	    XDR_find_type_name(in_s->type),
	    XDR_find_type_name(in_t->type));
#endif
    reason = "data does not match schema";
    goto serialize_failed_output;    
  }
  switch(in_s->type){
  case XDR_INT:
  case XDR_BOOL:
  case XDR_UINT:
    {
      XDR_tree_simple *t;
      t = XDR_TREE_SIMPLE(in_t);
      *((u_int32_t*)buf) = htonl(t->val.uintVal);
      if(-1==write(fd,buf,4))
	return XDR_IO_ERROR;

    };break;
  case XDR_FLOAT: 
    {
      XDR_tree_simple *t;
      t = XDR_TREE_SIMPLE(in_t);
      if(-1==write(fd,&(t->val.floatVal),4))
	return XDR_IO_ERROR;
    };break;
  case XDR_HYPER:
  case XDR_UHYPER:
  case XDR_DOUBLE:
  case XDR_LONGDOUBLE:
    /*
      Note: This is unsupported becaus i dont know how to correctly re-order
      ieee FP numbers (and i can't think why i would use long longs at
      the moment
    */
    return XDR_UNSUPPORTED;
  case XDR_FIXEDOPAQUE:
  case XDR_VAROPAQUE:   
  case XDR_STRING:
    {
      XDR_tree_str * t;
      
      size_t slen;
      unsigned char * data;
      
      t = XDR_TREE_STR(in_t);
      
      data = t->data;
      slen = t->len;;
      
      if(in_s->type!=XDR_FIXEDOPAQUE){ //FIXED OPAQUE has no length prefix
	*((u_int32_t*)buf) = htonl(slen);
	if(-1==write(fd,buf,4))
	  return XDR_IO_ERROR;
      }
      
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
      XDR_array * s;
      XDR_tree_compound* t;
      
      s = XDR_SCHEMA_ARRAY(in_s);
      t = XDR_TREE_COMPOUND(in_t);
      
      if(s->base.type==XDR_FIXEDARRAY&& 
	 s->num_elems!=t->nelems){
	rv=XDR_SCHEMA_VIOLATION;
	goto serialize_failed_output;
      }

      len = t->nelems;
      *((u_int32_t*)buf) = htonl(len);
      if(-1==write(fd,buf,4))
	return XDR_IO_ERROR;
      
      for(i =0;i<len;i++){
	if(XDR_OK!=(rv =XDR_serialize_elem(s->elem_type,
					   t->subelems[i],
					   fd)))
	  return rv;
	
      }
    }
    break;
  case XDR_STRUCT:
    {
      size_t nelems;
      int i;
      XDR_struct * s;
      XDR_tree_compound * t;
      
      t = XDR_TREE_COMPOUND(in_t);
      s = XDR_SCHEMA_STRUCT(in_s);
      
      if( s->num_elems!=
	  t->nelems){
	rv=XDR_SCHEMA_VIOLATION;	
	goto serialize_failed_output;
      }     
      nelems = s->num_elems;
      
      for(i = 0;i<nelems;i++){
	if(XDR_OK!=(rv=XDR_serialize_elem(s->elems[i],
					  t->subelems[i],fd)))
	  return rv;
      }
      
    }
    break;
  case XDR_UNION:
    {
      int i;
      unsigned int d_val;
      XDR_type_union *s;
      XDR_tree_compound * t;
      XDR_schema * d_t = NULL;
      s = XDR_SCHEMA_UNION(in_s);
      t = XDR_TREE_COMPOUND(in_t);
      if(t->nelems!=2 ){
	reason ="compound for union does not have only 2 elems";
	rv=XDR_SCHEMA_VIOLATION;
	goto serialize_failed_output;
      }
      if( XDR_t_get_comp_elem(t,0)->type!=XDR_UINT){
	reason = "union first elem is not a UINT";
	rv=XDR_SCHEMA_VIOLATION;
	goto serialize_failed_output;
      }
      d_val = XDR_TREE_SIMPLE(XDR_t_get_comp_elem(t,0))->val.uintVal;
      
      for(i=0;i<s->num_alternatives;i++){
	if(s->elems[i].d==d_val){
	  d_t = s->elems[i].t;
	  break;
	}
      }
      
      if(NULL==d_t ){
	sprintf(errbuf,
		"The union discriminator of %d did not"\
		" match a schema element\n",
		d_val);
		
	reason = errbuf;
	rv=XDR_SCHEMA_VIOLATION;
	goto serialize_failed_output;
	
      }
      
      if(XDR_t_get_comp_elem(t,1)->type!=d_t->type){
	sprintf(errbuf,
		"desired target of %s does not match given type of %s\n",
		XDR_find_type_name(d_t->type),
		XDR_find_type_name(XDR_t_get_comp_elem(t,1)->type));
	reason = errbuf;
	rv=XDR_SCHEMA_VIOLATION;
	goto serialize_failed_output;
	  
      }
      
      *((int*)buf) = htonl(d_val);
      if(-1==write(fd,buf,4))
	return XDR_IO_ERROR;
      
      if(XDR_OK!=(rv=XDR_serialize_elem(d_t,
					XDR_TREE_COMPOUND(t)->subelems[1],
					fd)))
	return rv;
    }
    break;
  case XDR_VOID:
    break;
  default:
    reason = "attempt to serialize unknown type";
    rv=XDR_SCHEMA_VIOLATION;
    goto serialize_failed_output;
  }
  
  return XDR_OK;
 serialize_failed_output:

#ifdef VERBOSE_DEBUG
  fprintf(stderr,"serialization failed because %s, "\
	  "while processing data elemm of type %s, and schema elem of type %s\n",
	  reason,
	  XDR_find_type_name(in_t->type),
	  XDR_find_type_name(in_s->type));
#endif
  return rv;
	  
	  
}
