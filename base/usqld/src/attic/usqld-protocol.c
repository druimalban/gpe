/**
   usqld-protocol.c 
   (c) 2002 owen cliffe <occ@cs.bath.ac.uk. 
   this code is distributed under the LGPL(v2)
   
   the protocol driver for the usqld network interface
   the protocol interface revolves around the usqld_pickle struct
   all packets are castable to that struct, 

   the wire interface is something like
   type|totallenght|....

   where ... is the contents of the packet, 
   strings are length-prefix with a 4-byte unsigned integer which is network
   byte order, 
   
   the whole protocol is similar to LDAP BER without types on primitives
   
   the use of "pickle" is due to a very long night were for some reason i 
   started to type pickle instead of packet, after about 100 edits of 
   s/pickle/packet/ i decided that my mind was probably trying to tell 
   me somethign and let it be. 

 */

#include "sys/types.h"
#include <pthread.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h> 
#include <string.h>
#include <pthread.h>
#include <assert.h>


#include "usqld-protocol.h"

#define PICKLE_CONNECT 0x0
#define PICKLE_DISCONNECT 0x1
#define PICKLE_QUERY 0x2
#define PICKLE_ERROR 0x3
#define PICKLE_OK 0x4
#define PICKLE_STARTROWS 0x5
#define PICKLE_ROW 0x6
#define PICKLE_EOF 0x7
#define PICKLE_MAX 0x8

#define PICKLE_HEAD_LENGTH 8


/*
 * structs for unpacked usqld pickles
 * these pickles have some correspondance to what goes over the wire, 
 */

/*
 * handler table types, 
 * the handler table allows simple dispatching of message-specific functionality
 * based upon the message type number.. 
 */

typedef  int  (*pickle_delete_function) (usqld_pickle*);
typedef  int  (*pickle_unpack_function) (unsigned int,size_t ,unsigned char *,usqld_pickle**) ;
typedef  int  (*pickle_pack_function) (usqld_pickle* ,size_t * , unsigned char ** );

typedef struct 
{
   unsigned int msg_type;
   pickle_unpack_function unpack;
   pickle_pack_function pack;
   pickle_delete_function del;
}pickle_type_handler;


/*
 * handler table definitions.
 */

int usqld_unpack_connect(unsigned int pickle_type,size_t pickle_len,unsigned char *,usqld_pickle**p);
int usqld_unpack_startrows(unsigned int pickle_type,size_t pickle_len,unsigned char *,usqld_pickle**p);
int usqld_unpack_query(unsigned int pickle_type,size_t pickle_len,unsigned char *,usqld_pickle**p);
int usqld_unpack_row(unsigned int pickle_type,size_t pickle_len,unsigned char *,usqld_pickle**p);
int usqld_unpack_error(unsigned int pickle_type,size_t pickle_len,unsigned char *,usqld_pickle**p);
int usqld_unpack_basic(unsigned int pickle_type,size_t pickle_len,unsigned char *,usqld_pickle**p);


int usqld_pack_connect(usqld_pickle* p,size_t * pickle_len, unsigned char ** buf);
int usqld_pack_startrows(usqld_pickle* p,size_t * pickle_len, unsigned char ** buf);
int usqld_pack_query(usqld_pickle* p,size_t * pickle_len, unsigned char ** buf);
int usqld_pack_row(usqld_pickle* p,size_t * pickle_len, unsigned char ** buf);
int usqld_pack_error(usqld_pickle* p,size_t * pickle_len, unsigned char ** buf);
int usqld_pack_basic(usqld_pickle* p,size_t * pickle_len, unsigned char ** buf);

int usqld_delete_connect(usqld_pickle*p);
int usqld_delete_startrows(usqld_pickle*p);
int usqld_delete_query(usqld_pickle*p);
int usqld_delete_row(usqld_pickle*p);
int usqld_delete_error(usqld_pickle*p);
int usqld_delete_basic(usqld_pickle*p);

pickle_type_handler usqld_dispatch_table[] =
{
   //pickle_type | de-marshall function |marshall function| free_function
     {PICKLE_CONNECT,usqld_unpack_connect,usqld_pack_connect,usqld_delete_connect},
     {PICKLE_DISCONNECT,usqld_unpack_basic,usqld_pack_basic,usqld_delete_basic},
     {PICKLE_QUERY,usqld_unpack_query,usqld_pack_query,usqld_delete_query},
     {PICKLE_ERROR,usqld_unpack_error,usqld_pack_error,usqld_delete_error},
     {PICKLE_OK,usqld_unpack_basic,usqld_pack_basic,usqld_delete_basic},
     {PICKLE_STARTROWS,usqld_unpack_startrows,usqld_pack_startrows,usqld_delete_startrows},
     {PICKLE_ROW,usqld_unpack_row,usqld_pack_row,usqld_delete_row},
     {PICKLE_EOF,usqld_unpack_basic,usqld_pack_basic,usqld_delete_basic}
};

/*
 * handler table implementations.
 * 
 * unpack functions start with the stream after the header of the pickle
 * the pickle header length therefor does not include the 
 * size of the header itself 
 */   
int usqld_unpack_string(size_t buf_len, 
			unsigned char * buf,
			size_t *out_str_len,
			char  ** out_str)
{
  size_t string_length = 0;
  char * new_string = NULL;
  
  assert(buf!=NULL && out_str_len !=NULL && out_str!=NULL);

  if(buf_len <4)
    return USQLD_BUFFER_OVERRUN;
  
  string_length = ntohl(*((int *)buf));
  new_string = (char *)malloc(string_length +1);
  if(NULL==new_string)
    return USQLD_PMALLOC;
  
  if(string_length!=0){
      memcpy(new_string,buf +4,string_length);
  }
  new_string[string_length] = '\0';

  *out_str_len =string_length;
  *out_str = new_string;
  return USQLD_MALLOC;
}

int usqld_unpack_strarray(size_t buf_len,
			  unsigned char * buf,
			  size_t * out_num_elems,
			  size_t ** out_elem_len_array,
			  char *** out_elems)
{
    
    unsigned char* pos=NULL,*buf_top = NULL;
    char ** fields=NULL;
    size_t * field_lengths = NULL;
    size_t num_fields;
    int errcode = USQLD_OK;

    assert(buf!=NULL && buf!=NULL && out_num_elems!=NULL && out_elems !=NULL && out_elem_len_array!=NULL);

    pos = buf;    
    if(pos +4 >=buf_top ){
      errcode = USQLD_BUFFER_OVERRUN;
      goto unpack_strarray_error;
    }
    
    num_fields = ntohl(*((int *)pos)); 
    pos +=4;
    
    
    if((num_fields * 4) +4>buf_len){
      errcode = USQLD_BUFFER_OVERRUN;
      goto unpack_strarray_error;
    }
     
    
    if(num_fields!=0){
      int i;
      field_lengths = (size_t * ) malloc(num_fields *sizeof(size_t));    
      if(NULL==field_lengths){
	errcode = USQLD_PMALLOC;
	goto unpack_strarray_error;
      }
      bzero(field_lengths,num_fields * sizeof(size_t));
      
      fields = (char ** ) malloc(num_fields * sizeof(char *));
      if(NULL==fields){
	errcode = USQLD_PMALLOC;
	goto unpack_strarray_error;
      }
      bzero(fields,num_fields * sizeof(char *));
      
      for(i=0;i<num_fields;i++)
	{
	  if(USQLD_OK !=(errcode=usqld_unpack_string(buf_top -pos,pos,field_lengths +i,fields +i))){
	    goto unpack_strarray_error;
	  }
	  pos +=4 + field_lengths[i]; //this is icky 
	}
    }

    *out_elem_len_array= field_lengths;
    *out_elems = fields;
    *out_num_elems= num_fields;
    return USQLD_OK;
    
 unpack_strarray_error:
    if(fields!=NULL){
      int i;
      for(i = 0;i<num_fields;i++)
	if(NULL!=fields[i])
	  free(fields[i]);
      free(fields);
    }
    if(field_lengths !=NULL){
      free(field_lengths);
    }
    return errcode;
}



int usqld_unpack_connect(unsigned int pickle_type,
			 size_t pickle_len,
			 unsigned char *pickle_buf,
			 usqld_pickle **out_p)
{
  usqld_connect_pickle * new_p=NULL;
  size_t constr_len=0;
  char *pos=NULL;
  char * constr= NULL;
  int errcode = USQLD_OK;

  assert(pickle_type==PICKLE_CONNECT && pickle_buf!=NULL &&out_p !=NULL);
  
  if(USQLD_OK!=(errcode=usqld_unpack_string(pickle_len,pickle_buf,&constr_len,&constr)))
    goto unpack_connect_error;
   
  
  new_p =(usqld_connect_pickle *) malloc(sizeof(usqld_connect_pickle));
  if(NULL==new_p){
    errcode = USQLD_MALLOC;
    goto unpack_connect_error;
  }
  
  bzero (new_p,sizeof(usqld_connect_pickle));
  new_p->p.type =PICKLE_CONNECT;
  new_p->database_len = constr_len;
  new_p->database = constr;
  *out_p = (usqld_pickle*) new_p;
  return USQLD_OK;
  
 unpack_connect_error:
  if(NULL!=constr)
    free(constr);

  return errcode;
  
}


int usqld_unpack_startrows(unsigned int pickle_type,
			   size_t pickle_len,
			   unsigned char * pickle_buf,
			   usqld_pickle**out_p)
{
  usqld_startrows_pickle * new_p=NULL;
  size_t *field_lengths =NULL;
  char ** field_titles = NULL;
  size_t num_fields =0;
  int errcode = USQLD_OK;
  
  assert(pickle_type==PICKLE_STARTROWS);   
  assert(pickle_buf !=NULL);
  assert(out_p!=NULL);
  
  if(USQLD_OK !=
     (errcode=usqld_unpack_strarray(pickle_len,
				   pickle_buf,
				   &num_fields,
				   &field_lengths,
				   &field_titles))){
    goto unpack_startrows_error;
  }
  
  new_p = (usqld_startrows_pickle*)malloc(sizeof(usqld_startrows_pickle));
  if(new_p==NULL)
    {
      errcode = USQLD_MALLOC;
      goto unpack_startrows_error;
    }
    
  bzero(new_p,sizeof(usqld_startrows_pickle));
  new_p->p.type = PICKLE_STARTROWS;
  new_p->num_fields = num_fields;
  new_p->field_lengths = field_lengths;
  new_p->field_titles = field_titles;
  *out_p = (usqld_pickle*) new_p;
 unpack_startrows_error:
  if(NULL!=new_p){
    free(new_p);
  }
  
  
  if(NULL!=field_titles){
    int i;
    for(i=0;i<num_fields;i++)
      if(NULL!=field_titles[i]){
	free(field_titles[i]);
      }
    free(field_titles);
  }
  if(field_lengths)
    free(field_lengths);

  return errcode;
}


 int usqld_unpack_query(unsigned int pickle_type,
			size_t pickle_len,
			unsigned char *pickle_buf,
			usqld_pickle**out_p){

   usqld_query_pickle * new_p = NULL;
   int errcode =USQLD_OK;
   char * query_str = NULL;
   size_t query_str_len = 0;

   assert(pickle_type==PICKLE_QUERY && pickle_buf !=NULL && out_p !=NULL);
   
   if(USQLD_OK!=(errcode=usqld_unpack_string(pickle_len,
					     pickle_buf,
					     &query_str_len,
					     &query_str))){
     goto unpack_query_error;
   }
   
   new_p = (usqld_query_pickle*) malloc(sizeof(usqld_query_pickle));
   if(NULL==new_p){
     errcode = USQLD_MALLOC;
     goto unpack_query_error;
   }
   bzero(new_p,sizeof(usqld_unpack_query));
   new_p->p.type = PICKLE_QUERY;
   new_p->query = query_str;
   new_p->query_len = query_str_len;
   *out_p =(usqld_pickle*) new_p;
   return USQLD_OK;
   
 unpack_query_error:
   if(NULL!=query_str)
     free(query_str);
   
   if(NULL!=new_p)
     free(new_p);
   return errcode;
  
}

int usqld_unpack_row(unsigned int pickle_type,
		     size_t pickle_len,
		     unsigned char *pickle_buf,
		     usqld_pickle**out_p){


  usqld_row_pickle * new_p = NULL;
  size_t num_fields;
  size_t * field_lengths = 0;
  char ** fields = NULL;
  int errcode = USQLD_OK;
   
  assert(pickle_type==PICKLE_ROW && pickle_buf !=NULL && out_p !=NULL);  
 
  if(USQLD_OK !=(errcode=usqld_unpack_strarray(pickle_len,
					      pickle_buf,
					      &num_fields,
					      &field_lengths,
					      &fields))){
    goto unpack_row_error;
  }
  
  new_p = (usqld_row_pickle*)malloc(sizeof(usqld_row_pickle));
  if(NULL==new_p){
    errcode = USQLD_MALLOC;
    goto unpack_row_error;
  }
    
  bzero(new_p,sizeof(usqld_row_pickle));
  new_p->p.type = PICKLE_ROW;
  new_p->num_fields = num_fields;
  new_p->field_lengths = field_lengths;
  new_p->fields = fields;
  
  *out_p = (usqld_pickle*)new_p;

  return USQLD_OK;
  
 unpack_row_error:
  if(NULL!=field_lengths)
    free(field_lengths);
  
  if(NULL!=fields){
    int i;
    for(i =0;i<num_fields;i++){
      if(NULL!=fields[i])
	free(fields[i]);
      
    }
  }
  return errcode;
}


int usqld_unpack_error(unsigned int pickle_type,
		       size_t pickle_len,
		       unsigned char * pickle_buf,
		       usqld_pickle**out_p){
   usqld_error_pickle * new_p = NULL;
   int errcode =USQLD_OK;
   char * error_str = NULL;
   size_t error_str_len = 0;
   int the_errcode = 0;
   unsigned char * pos =0;
   
   assert(pickle_type==PICKLE_ERROR && pickle_buf !=NULL && out_p !=NULL);
   
   pos = pickle_buf;
   
   the_errcode = ntohl(*((int *)pos));
   pos +=4;
   if(USQLD_OK!=(errcode=usqld_unpack_string(pickle_len -4,
					     pos,
					     &error_str_len,
					     &error_str))){
     goto unpack_query_error;
   }
   
   new_p = (usqld_error_pickle*) malloc(sizeof(usqld_error_pickle));
   if(NULL==new_p){
     errcode = USQLD_MALLOC;
     goto unpack_query_error;
   }
   bzero(new_p,sizeof(usqld_error_pickle));

   new_p->p.type = PICKLE_ERROR;
   new_p->errcode = the_errcode;
   new_p->errstring_len = error_str_len;
   new_p->errstring =error_str;

   *out_p =(usqld_pickle*) new_p;
   return USQLD_OK;
   
 unpack_query_error:
   if(NULL!=error_str)
     free(error_str);
   
   if(NULL!=new_p)
     free(new_p);
   return errcode;
    
}

int usqld_unpack_basic(unsigned int pickle_type,
		       size_t pickle_len,
		       unsigned char *pickle_buf,
		       usqld_pickle**out_p){
  
  usqld_pickle * new_p;
  
  new_p = (usqld_pickle*)malloc(sizeof(usqld_pickle));

  if(NULL==new_p){
    return USQLD_MALLOC;
  }
  
  new_p->type = pickle_type;
  *out_p = new_p;
  return USQLD_OK;
}

int usqld_pack_bytes(unsigned char **buf, //in/out the buffer pointer
		     size_t * buf_len, //in/out the length remaining 
		     unsigned char * data, //in
		     size_t data_len//in
		     ){ 
  assert(data_len <= *buf_len);
  memcpy(*buf,data,data_len);
  *buf+=data_len;
  buf_len -=data_len;
  return USQLD_OK;
}
		     

int usqld_pack_connect(usqld_pickle* p,
		       size_t * pickle_len, 
		       unsigned char ** out_buf){
  size_t buf_size=0,remaining_size =0;
  unsigned char * buf = NULL;
  unsigned char * pos =NULL;
  int errcode = 0;
  unsigned char intbuf[4];
  usqld_connect_pickle *real_p = (usqld_connect_pickle *) p;
  
  assert(p->type==PICKLE_CONNECT);
 
  remaining_size =buf_size = 4+real_p->database_len;
  buf = (unsigned char*) malloc(buf_size);
  if(NULL==buf){
    errcode = USQLD_MALLOC;
  }
  pos = buf;
  *intbuf = htonl(real_p->database_len);
  
  if(USQLD_OK!=(errcode=usqld_pack_bytes(&pos,&remaining_size,intbuf,4)))
    goto pack_connect_error;
  
  if(USQLD_OK!=(errcode=usqld_pack_bytes(&pos,&remaining_size,real_p->database,real_p->database_len)))
    goto pack_connect_error;

  assert(remaining_size==0);
  
  *pickle_len = buf_size;
  *out_buf = buf;
  return USQLD_OK;

 pack_connect_error:
  if(buf!=NULL)
    free(buf);
  
  return errcode;
  
}

int usqld_pack_startrows(usqld_pickle* p,size_t * pickle_len, unsigned char ** out_buf){
  size_t buf_size=0,remaining_size =0;
  unsigned char * buf = NULL;
  unsigned char * pos =NULL;
  int errcode = 0;
  unsigned char intbuf[4];
  int i;

  usqld_startrows_pickle *real_p = (usqld_startrows_pickle *) p;
  
  assert(p->type==PICKLE_STARTROWS);
 
  
  buf_size = 4+(4*real_p->num_fields);
  for(i = 0;i<real_p->num_fields;i++){
    buf_size +=real_p->field_lengths[i];
  }
  remaining_size = buf_size;

  buf = (unsigned char*) malloc(buf_size);
  if(NULL==buf){
    errcode = USQLD_MALLOC;
  }

  pos = buf;
  *intbuf = htonl(real_p->num_fields);
  
  if(USQLD_OK!=(errcode=usqld_pack_bytes(&pos,&remaining_size,intbuf,4)))
    goto pack_connect_error;

  for(i= 0;i<real_p->num_fields;i++){
    *intbuf = htonl(real_p->field_lengths[i]);
    if(USQLD_OK!=(errcode=usqld_pack_bytes(&pos,&remaining_size,intbuf,4)))
      goto pack_connect_error;
    
    if(USQLD_OK!=(errcode=usqld_pack_bytes(&pos,&remaining_size,real_p->field_titles[i],real_p->field_lengths[i])))
      goto pack_connect_error;
  
  }

  assert(remaining_size==0);
  
  *pickle_len = buf_size;
  *out_buf = buf;
  return USQLD_OK;

 pack_connect_error:
  if(buf!=NULL)
    free(buf);
  
  return errcode;  
}

int usqld_pack_query(usqld_pickle* p,size_t * pickle_len, unsigned char ** out_buf){
  size_t buf_size=0,remaining_size =0;
  unsigned char * buf = NULL;
  unsigned char * pos =NULL;
  int errcode = 0;
  unsigned char intbuf[4];
  usqld_query_pickle *real_p = (usqld_query_pickle *) p;
  
  assert(p->type==PICKLE_QUERY);
 
  remaining_size =buf_size = 4+real_p->query_len;
  buf = (unsigned char*) malloc(buf_size);
  if(NULL==buf){
    errcode = USQLD_MALLOC;
  }
  pos = buf;
  *intbuf = htonl(real_p->query_len);
  
  if(USQLD_OK!=(errcode=usqld_pack_bytes(&pos,&remaining_size,intbuf,4)))
    goto pack_query_error;
  
  if(USQLD_OK!=(errcode=usqld_pack_bytes(&pos,&remaining_size,real_p->query,real_p->query_len)))
    goto pack_query_error;

  assert(remaining_size==0);
  
  *pickle_len = buf_size;
  *out_buf = buf;
  return USQLD_OK;

 pack_query_error:
  if(buf!=NULL)
    free(buf);
  
  return errcode;
  
}

int usqld_pack_row(usqld_pickle* p,size_t * pickle_len, unsigned char ** out_buf){
  size_t buf_size=0,remaining_size =0;
  unsigned char * buf = NULL;
  unsigned char * pos =NULL;
  int errcode = 0;
  unsigned char intbuf[4];
  int i;

  usqld_row_pickle *real_p = (usqld_row_pickle *) p;
  
  assert(p->type==PICKLE_ROW);
 
  
  buf_size = 4+(4*real_p->num_fields);
  for(i = 0;i<real_p->num_fields;i++){
    buf_size +=real_p->field_lengths[i];
  }
  remaining_size = buf_size;

  buf = (unsigned char*) malloc(buf_size);
  if(NULL==buf){
    errcode = USQLD_MALLOC;
  }

  pos = buf;
  *intbuf = htonl(real_p->num_fields);
  
  if(USQLD_OK!=(errcode=usqld_pack_bytes(&pos,&remaining_size,intbuf,4)))
    goto pack_connect_error;

  for(i= 0;i<real_p->num_fields;i++){
    *intbuf = htonl(real_p->field_lengths[i]);
    if(USQLD_OK!=(errcode=usqld_pack_bytes(&pos,&remaining_size,intbuf,4)))
      goto pack_connect_error;
    
    if(USQLD_OK!=(errcode=usqld_pack_bytes(&pos,&remaining_size,real_p->fields[i],real_p->field_lengths[i])))
      goto pack_connect_error;
    
  }
  
  assert(remaining_size==0);
  
  *pickle_len = buf_size;
  *out_buf = buf;
  return USQLD_OK;
  
 pack_connect_error:
  if(buf!=NULL)
    free(buf);
  
  return errcode; 
}

int usqld_pack_error(usqld_pickle* p,size_t * pickle_len, unsigned char ** out_buf)
{
  size_t buf_size=0,remaining_size =0;
  unsigned char * buf = NULL;
  unsigned char * pos =NULL;
  int errcode = 0;
  unsigned char intbuf[4];
  usqld_error_pickle *real_p = (usqld_error_pickle *) p;
  
  assert(p->type==PICKLE_ERROR);
  
  remaining_size =buf_size = 8+real_p->errstring_len;
  buf = (unsigned char*) malloc(buf_size);
  if(NULL==buf){
    errcode = USQLD_MALLOC;
  }
  
  pos = buf;

  *intbuf = htonl(real_p->errcode);
  
  if(USQLD_OK!=(errcode=usqld_pack_bytes(&pos,&remaining_size,intbuf,4)))
    goto pack_error_error;
  
  *intbuf = htonl(real_p->errstring_len);
  if(USQLD_OK!=(errcode=usqld_pack_bytes(&pos,&remaining_size,intbuf,4)))
    goto pack_error_error;
  
  if(USQLD_OK!=(errcode=usqld_pack_bytes(&pos,&remaining_size,real_p->errstring,real_p->errstring_len)))
    goto pack_error_error;
  
  assert(remaining_size==0);
  
  *pickle_len = buf_size;
  *out_buf = buf;
    return USQLD_OK;
    
 pack_error_error:
    if(buf!=NULL)
      free(buf);
    
  return errcode;
  
}

int usqld_pack_basic(usqld_pickle* p,size_t * pickle_len, unsigned char ** buf){
  *pickle_len = 0;
  return USQLD_OK;
}

int usqld_delete_connect(usqld_pickle*p){
  usqld_connect_pickle * real_p = (usqld_connect_pickle*) p;
  assert(NULL!=p && p->type==PICKLE_CONNECT);
  
  if(NULL!=real_p->database)
    free(real_p->database);
  
  free(real_p);
  return USQLD_OK;
}

int usqld_delete_startrows(usqld_pickle*p){
  usqld_startrows_pickle * real_p = (usqld_startrows_pickle*)p;
  
  assert(NULL!=p && p->type==PICKLE_STARTROWS);
  if(NULL!=real_p->field_titles){
    int i;
    
    for(i = 0;i<real_p->num_fields;i++){
      if(real_p->field_titles[i])
	free(real_p->field_titles[i]);
    }
    
  }
  if(NULL!=real_p->field_lengths)
    free(real_p->field_lengths);
  
  return USQLD_OK;
}

int usqld_delete_query(usqld_pickle*p){ 
  usqld_query_pickle * real_p = (usqld_query_pickle*) p;
  assert(NULL!=p && p->type==PICKLE_QUERY);
  
  if(NULL!=real_p->query)
    free(real_p->query);
  
  free(real_p);
  return USQLD_OK; 
}

int usqld_delete_row(usqld_pickle*p){
  usqld_row_pickle * real_p = (usqld_row_pickle*)p;
  
  assert(NULL!=p && p->type==PICKLE_ROW);
  if(NULL!=real_p->fields){
    int i;
    for(i = 0;i<real_p->num_fields;i++){
      if(real_p->fields[i])
	free(real_p->fields[i]);
    }
    
  }
  if(NULL!=real_p->field_lengths)
    free(real_p->field_lengths);
  
  return USQLD_OK;
}

int usqld_delete_error(usqld_pickle*p){
  usqld_error_pickle * real_p = (usqld_error_pickle*) p;
  assert(NULL!=p && p->type==PICKLE_ERROR);
  
  if(NULL!=real_p->errstring)
    free(real_p->errstring);
  
  free(real_p);
  return USQLD_OK;   
}

int usqld_delete_basic(usqld_pickle*p){

  assert(NULL!=p );
  free(p);
  return USQLD_OK;   
}


void uslqd_pickle_free(usqld_pickle*p)
{
  usqld_pickle * real_p = (usqld_pickle*) p;
  
  assert(NULL!=real_p && real_p->type<PICKLE_MAX);
  usqld_dispatch_table[real_p->type].del(real_p);
}

int usqld_pack_pickle(usqld_pickle * p,size_t  *pickle_len,unsigned char **buf){
  assert(NULL!=p && p->type<PICKLE_MAX);
  return usqld_dispatch_table[p->type].pack(p,pickle_len,buf);
}

int usqld_unpack_pickle(unsigned int pickle_type,
			size_t pickle_len,
			unsigned char *buf,
			usqld_pickle ** p){
    
  assert(pickle_type < PICKLE_MAX);
	 
  return usqld_dispatch_table[pickle_type].unpack(pickle_type,
					      pickle_len,
					      buf,
					      p);
}



int usqld_send_pickle(int fd,usqld_pickle *p)
{
  usqld_pickle * real_p = (usqld_pickle *)p;
  unsigned char * pickle_buf=NULL;
  size_t pickle_len=0;
  int errcode = 0;
  unsigned char  header[8];
  
  
  assert(NULL!=p && fd!=-1); 
  if(USQLD_OK !=(errcode =usqld_pack_pickle(real_p,&pickle_len,&pickle_buf))){
    goto send_pickle_finish;
  }
  
  *header = htonl(real_p->type);
  *(((int*)header )+ 1)  = htonl(pickle_len);
  
  if(-1==send(fd,header,8,0)){
    errcode = USQLD_SEND;
    goto send_pickle_finish;
  }
  
  if(-1==send(fd,pickle_buf,pickle_len,0)){
    errcode = USQLD_SEND;
    goto send_pickle_finish;
  }else{
    errcode = USQLD_OK;
  }
    
  
 send_pickle_finish:
  if(pickle_buf!=NULL)
    free(pickle_buf);
  return errcode;
}

int usqld_recv_pickle(int fd,
		      usqld_pickle **p)
{
  usqld_pickle * recv_p;
  char header[8];
  size_t msg_len = 0;
  unsigned int msg_type = 0;
  unsigned char * msg_buf = NULL;
  int errcode = 0;

  assert(fd!=-1 && NULL!=p);

  if(-1==recv(fd,header,8,MSG_WAITALL)){
    return USQLD_RECV;
  }
  msg_type = ntohl(*((int*)header));
  msg_len  = ntohl(*(((int*)header)+1));
  

  if(msg_type >=PICKLE_MAX)
    return USQLD_QUE;
  
  if(NULL==(msg_buf = (unsigned char *)malloc(msg_len))){
    return USQLD_PMALLOC;
  }
  bzero(msg_buf,msg_len);
  
  if(-1==(recv(fd,msg_buf,msg_len,MSG_WAITALL))){
    free(msg_buf);
    return USQLD_RECV;
  }
  
  if(USQLD_OK !=(errcode=usqld_unpack_pickle(msg_type,
					     msg_len,
					     msg_buf,
					     &recv_p))){
    free(msg_buf);
    return errcode;
  }
  
  *p = (usqld_pickle*) recv_p;
  return USQLD_OK;
}


