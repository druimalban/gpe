#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <xdr/xdr.h>
#include "httplike.h"


// this is a transition relation, 
// ANY STATES ABOVE HTTPLIKE_STATE_MAX_T have no out transitions, so aren't included. 
struct httplike_trans lex_trans [11][4]=
  {   //NWS-WC-colon        NEWLINE        COLON            WHITESPACE
    {{HTPL_IS_OPER,1},{HTPL_IS_ERRO,0},{HTPL_IS_ERRO,0},{HTPL_IS_ERRO,0}}, //HTPL_IS_INIT
    {{HTPL_IS_OPER,1},{HTPL_IS_ERRO,0},{HTPL_IS_OPER,1},{HTPL_IS_WTS1,0}}, //HTPL_IS_OPER
    {{HTPL_IS_OPND,1},{HTPL_IS_ERRO,0},{HTPL_IS_OPND,1},{HTPL_IS_WTS1,0}}, //HTPL_IS_WTS1
    {{HTPL_IS_OPND,1},{HTPL_IS_ERRO,0},{HTPL_IS_OPND,1},{HTPL_IS_WTS2,0}}, //HTPL_IS_OPND 
    {{HTPL_IS_VERS,1},{HTPL_IS_H_NL,0},{HTPL_IS_VERS,1},{HTPL_IS_WTS2,0}}, //HTPL_IS_WTS2
    {{HTPL_IS_HNAM,1},{HTPL_IS_DATA,0},{HTPL_IS_ERRO,0},{HTPL_IS_WTS3,0}}, //HTPL_IS_H_NL
    {{HTPL_IS_HNAM,1},{HTPL_IS_ERRO,0},{HTPL_IS_ERRO,0},{HTPL_IS_WTS3,0}}, //HTPL_IS_WTS3
    {{HTPL_IS_HNAM,1},{HTPL_IS_ERRO,0},{HTPL_IS_WTS4,0},{HTPL_IS_COLO,0}}, //HTPL_IS_HNAM
    {{HTPL_IS_ERRO,0},{HTPL_IS_ERRO,0},{HTPL_IS_WTS4,0},{HTPL_IS_COLO,0}}, //HTPL_IS_COLO
    {{HTPL_IS_HVAL,1},{HTPL_IS_H_NL,0},{HTPL_IS_HVAL,1},{HTPL_IS_WTS4,0}}, //HTPL_IS_WTS4
    {{HTPL_IS_HVAL,1},{HTPL_IS_H_NL,0},{HTPL_IS_HVAL,1},{HTPL_IS_HVAL,1}}, //HTPL_IS_HVAL
  };    

#define p_set(d, p, v) p_set_impl((void*)d,(void**)p,(void*)v)

void * p_set_impl(void * d,void** p,void * v){
  void * out_p;
  assert(d!=NULL);
  out_p = *p;
  *p = v;
  return out_p;
}

#define p_get(d,v) p_get_impl((void *)d,(void**)v)

void* p_get_impl(void * d, void **p){
  assert(d!=NULL);
  return *p;
}


int httplike_socket_set_timeout(httplike_socket * sock,int timeout){
  return (int) p_set(sock,&(sock->timeout),timeout);
}

int httplike_socket_get_timeout(httplike_socket * sock){
  return (int) p_get(sock,&(sock->timeout));
}

httplike_message_func  httplike_socket_set_message_func(httplike_socket * sock,
							httplike_message_func func){
  return (httplike_message_func) p_set(sock,&(sock->on_dispatch),func);
}

httplike_message_func  httplike_socket_get_message_func(httplike_socket * sock){
  return (httplike_message_func) p_get(sock,(&sock->on_dispatch));
}

httplike_timeout_func  httplike_socket_set_timeout_func(httplike_socket * sock,
							httplike_timeout_func func){
  return (httplike_timeout_func)p_set(sock,&(sock->on_timeout),func);
}

httplike_timeout_func  httplike_socket_get_timeout_func(httplike_socket * sock){
  return (httplike_timeout_func) p_get(sock,&(sock->on_timeout));
}

httplike_error_func  httplike_socket_set_error_func(httplike_socket * sock,
						    httplike_error_func func){
  return (httplike_error_func)p_set(sock,&(sock->on_error),func);
}

httplike_error_func  httplike_socket_get_error_func(httplike_socket * sock){
  return (httplike_error_func)p_get(sock,&(sock->on_error));
}


void httplike_parse_error(httplike_socket * sock,
			  httplike_parse_context * pc,
			  int err, const char * msg){
  assert(NULL!=sock);
  pc->istate = HTPL_IS_ERRO;
  if(sock->on_error)
    sock->on_error(sock,err,msg);
}

httplike_socket* httplike_new_socket(int fd){

  httplike_socket * sock;

  sock = XDR_malloc(httplike_socket);
  bzero(sock,sizeof(httplike_socket));
  sock->fd = fd;
  return sock;
}

int whitespace(char c){
  return (c==' ' || c=='\t');
}

/**
   re-initializes the parsebuf to a sensible size and empties it.
   allows for the parse buffer being null. 
 */
void pc_buffer_reset(httplike_parse_context * pc){
  assert(NULL!=pc);

  pc->c_term_len =0;

  if(pc->parsebuf_size > HEADER_BUF_INIT){
    free(pc->parsebuf);
    pc->parsebuf = NULL;
  }
    
  if(pc->parsebuf==NULL){
    pc->parsebuf = malloc(HEADER_BUF_INIT);
    assert(pc->parsebuf!=NULL);
    pc->parsebuf_size = HEADER_BUF_INIT;
  }
}

/**
   resets the parse context complete (and frees any packet contents) after a dispatch
*/
void pc_packet_reset(httplike_parse_context * pc){
  httplike_packet *p;
  assert(NULL!=pc);
  p = &(pc->packet);
  pc_buffer_reset(pc);
  
  if(p->operation)
    XDR_free(p->operation);
  
  if(p->operand)
    XDR_free(p->operand);
  if(p->version)
    XDR_free(p->version);
  if(p->content)

    XDR_free(p->content);
  
  p->content_len = 0;
  p->nheader = 0;
    
}

/*
  dispatches the current contents of the parse buffer into a string,
  and then resets the buffer
 */
char * string_term_chomp(httplike_parse_context *pc){
  char * new_str=NULL;
  
  new_str = malloc(pc->c_term_len +1); 
  
  assert(NULL!=new_str);
  
  strncpy(new_str,pc->parsebuf,pc->c_term_len+1);
  
  pc_buffer_reset(pc);
  return new_str;
}

/*
  handles a lexer transition from state to next
  
 */
void httplike_handle_state_exit(httplike_socket * sock,
			   httplike_parse_context * pc,
			   httplike_internal_state state,
			   httplike_internal_state next){
  httplike_packet *  packet =  &(pc->packet);
  
  switch (state){
  
  case HTPL_IS_INIT: 
    // pc->packet = packet = XDR_malloc(httplike_packet);
    // assert(packet);
    // bzero(packet,sizeof(httplike_packet));
    break;

  case HTPL_IS_OPER: 
    assert(packet->operation);
    packet->operation = string_term_chomp(pc);
    break;

  case HTPL_IS_OPND:
    assert(packet->operand);
    packet->operand = string_term_chomp(pc);
    break;

  case HTPL_IS_VERS:
    assert(packet->version);
    packet->version = string_term_chomp(pc);
    break;
    
  case HTPL_IS_HNAM:
    if(packet->nheader >= HTTPLIKE_MAX_HEADERS){
      httplike_parse_error(sock,pc,HTTPLIKE_ERR_TOOMANY_HEADERS,"She cannie take it any more capn");
      break;
    }
    
    packet->nheader++;
    packet->headers[packet->nheader].h_name = string_term_chomp(pc);
    break;
  case HTPL_IS_HVAL:
    packet->headers[packet->nheader].h_val = string_term_chomp(pc);
    if(strcasecmp(HTTPLIKE_HEADER_CONTENT_LEN,packet->headers[packet->nheader].h_name)==0){
      if(sscanf(packet->headers[packet->nheader].h_val,"%d",
		&(packet->content_len))!=1){
	httplike_parse_error(sock,pc,HTTPLIKE_ERR_MALF_HEADER,
			     "The content length header value did not parse as a natural number..");
	
      }
    }
    break;
    
  case HTPL_IS_DATA:
    packet->content = pc->parsebuf;
    if(sock->on_dispatch)
      sock->on_dispatch(sock,packet);
    pc_packet_reset(pc);
    break;
  }
}


/*
  this is basically the lexer driver, 
  takes some content from somewhere, and lexes it, calling
  httplike_handle_state_exit when it is done.
*/
void httplike_consume_content(httplike_socket * sock, 
			      char * data, unsigned int data_len){
  char *c;
  httplike_parse_context * pc;
  httplike_trans * c_trans;
  pc = &(sock->pc);

  for(c =data;c<data + data_len;c++,data_len--){
    if(pc->istate == HTPL_IS_ERRO){
      return;
      break;
    }else if(pc->istate > HTPL_IS_MAX_T){
      assert(1);
    }
    

    if(pc->istate==HTPL_IS_DATA){
      size_t to_copy = ((pc->packet.content_len - pc->c_term_len) < data_len? pc->packet.content_len-pc->c_term_len : data_len);
            
      //we consume as much data as we are allowed for the data segment. 
      //and then manipulate c to suit. 
      
      if(pc->parsebuf_size <(pc->c_term_len + to_copy)){
	//grow the parsebuf if we need to
	pc->parsebuf = realloc(pc->parsebuf,pc->c_term_len + to_copy);
	assert(pc->parsebuf!=NULL);
      }
      
      memcpy(pc->parsebuf + pc->c_term_len,data,to_copy);
      pc->c_term_len+=to_copy;
      if(pc->packet.content_len == pc->c_term_len){
	httplike_handle_state_exit(sock,pc,HTPL_IS_DATA,HTPL_IS_INIT);
	pc->istate = HTPL_IS_INIT;
      }
      c+=to_copy;
    }else{
      
      if(whitespace(*c)){
	c_trans = &(lex_trans[pc->istate][HTTPLIKE_TRIGGER_WHITESPACE]);
      }else if(*c==':'){
	c_trans = &(lex_trans[pc->istate][HTTPLIKE_TRIGGER_COLON]);
      }else if (*c=='\n'){
	c_trans = &(lex_trans[pc->istate][HTTPLIKE_TRIGGER_NEWLINE]);
      }else{
	c_trans = &(lex_trans[pc->istate][HTTPLIKE_TRIGGER_NONWSPACE]);
      }
      
      if(c_trans->next != pc->istate){
	httplike_handle_state_exit(sock,pc,pc->istate,c_trans->next);
      }

      if(c_trans->chomp){
	pc->parsebuf[pc->c_term_len] = *c;
	pc->c_term_len++;
      }
    }
  }
}


  

void httplike_pump_socket(httplike_socket *sock){
  
  assert(sock!=NULL);
  
  if(sock->state!=HTTPLIKE_STATE_CLOSED){
    int nb;
    httplike_parse_context * pc; 
    pc = &(sock->pc);
    char netbuf[HTTPLIKE_NET_BUF_LEN];
    while((nb=recv(sock->fd,netbuf,HTTPLIKE_NET_BUF_LEN,MSG_DONTWAIT))){
      httplike_consume_content(sock,netbuf,nb);
    }
  }
}


int httplike_socket_error(httplike_socket *sock){
  return (sock->pc.istate==HTPL_IS_ERRO);
}

int httplike_socket_busy(httplike_socket *sock){
  assert(sock!=NULL);
  
  return (sock->pc.istate==HTPL_IS_INIT);
}

int httplile_socket_fd(httplike_socket * sock){
  assert(sock);
  return(sock->fd);
}

int httplike_socket_destroy(httplike_socket* sock){
  assert(sock);
  
}


void * httplike_socket_get_data(httplike_socket * sock){
  assert(sock);
  return sock->data;
  
}

void * httplike_socket_set_data(httplike_socket * sock, voiid * data){
  void * data;
  assert(sock);
  
}
httplike_data_destructor_func httplike_socket_get_data_destructor(httplike_socket * sock){
  assert(sock);
  return sock->dest_data;
}
