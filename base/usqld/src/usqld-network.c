#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <httplike/httplike.h>

#include "xdr.h"
#include "usqld-network.h"
#include "usqld-protocol.h"


/**
   recieves a whole packet (synchronously) 

   **BUG** in a perfect world this would not block ever, as this is bad.
   we get away with it because packets tend to be small.
   technically this is going need a re-write of the XDR parser, into 
   a re-entrant stateful parser, which could be done. 
   
   sock: a socket
   packet: where to put the recieved packet will always be NULL
   if the function returns anything  byt USQLD_OK;
   
   return value: 
 */

int usqld_packet_handler(httplike_socket *sock,httplike_packet*packet){

  void (*handler) (int);
  int rv;
  XDR
  XDR_tree * xdr_packet;

  handler = signal(SIGPIPE,SIG_IGN);

  rv  = XDR_deserialize_elem(usqld_get_protocol(),sock->fd,packet);
  signal(SIGPIPE,handler);
#ifdef VERBOSE_DEBUG
   if(rv==USQLD_OK)
     {

       fprintf(stderr,"got a %s packet\n",
	       usqld_find_packet_name(
	          usqld_get_packet_type(usqld_envelope_get_packet(*packet))));
       
       XDR_tree_dump(*packet);
     }else
       {
	 fprintf(stderr,"packet recieve failed with code %d\n",rv);
       }
#endif   
   return rv;
}


/**
  wraps a packet in a session header.
 */
usqld_envelope * usqld_wrap_packet(usqld_session * session, 
				 usqld_packet * packet_in){
  unsigned int session_id; 
  usqld_packet * packet_ev_contents[2];
  usqld_envelope * packet;
  
  assert(session!=NULL);
  
  session_id = session->session_id;
  
  packet_ev_contents[0] = XDR_tree_new_uint(session_id);
  packet_ev_contents[1] = packet_in;
  packet = XDR_tree_new_struct(2,packet_ev_contents);
  
  return packet;
}

/**
   takes a protocol packet, wraps it in a session header, dispatches
   it and frees the packet.
*/                                                       
int usqld_session_send_packet(usqld_socket * socket,
			      usqld_session * session, 
			      usqld_packet * packet_in){
  int rv;
  usqld_envelope * env;
  env = usqld_wrap_packet(session,packet_in);
  rv = usqld_send_packet(socket,env);
  XDR_tree_free(env);
  return rv;
}

/**
   sends a packet over the wire synchronously
   sock: is a socket to send a packet
 */
int usqld_send_packet(usqld_socket * sock,usqld_envelope * packet){

  void (*handler) (int);
  int rv;


  
#ifdef VERBOSE_DEBUG
  fprintf(stderr,"about to send a %s packet\n",
	  usqld_find_packet_name(XDR_t_get_union_disc(XDR_TREE_COMPOUND(packet))));
   XDR_tree_dump(packet);
#endif

   handler = signal(SIGPIPE,SIG_IGN);
   rv =  XDR_serialize_elem(usqld_get_protocol(),packet,sock->fd);
   
   signal(SIGPIPE,handler);
   return rv;
}


/**
  checks Incoming stream to see if a packet header is there
  return the the start (i.e. the type) of the incomming packet
  used to deal with an interrupt send during a packet transmit
 */
unsigned int usqld_socket_data_waiting(usqld_socket* socket){
  unsigned char header[4];
  
  if(4== recv(socket->fd,header,4,MSG_DONTWAIT|MSG_PEEK)){
    return ntohl(*((unsigned int *)header));
  }
  return 0;
}


usqld_session_store * usqld_session_store_new();

/**
   Creates a new socket from an existing FD
 */
usqld_socket * usqld_socket_new_from_fd(int fd){
  usqld_socket * sock;
  sock = XDR_malloc(usqld_socket);
  bzero(sock,sizeof(usqld_socket));
  sock->next_session_id =1;
  sock->sessions =  usqld_session_store_new();
  sock-> fd = fd;
  return sock;
}

/**
   Creates a new session on top of an existing socket.
   This session gets a new unique session ID. 
 */
usqld_session* usqld_socket_new_session(usqld_socket *socket){
  unsigned int session_id;
  assert(NULL!=socket);
  session_id = socket->next_session_id;  
 
  while(NULL!=usqld_session_store_find(socket->sessions,session_id)){
    session_id++;
  }

  socket->next_session_id = session_id +1;
  
  return usqld_socket_establish_session(socket,session_id);
}

usqld_session* usqld_socket_establish_session(usqld_socket *socket,unsigned int session_id){
  usqld_session * session;

  assert(NULL!=socket);  

  if(usqld_session_store_find(socket->sessions,session_id))
    return NULL;
  
  session = XDR_malloc(usqld_session);
  bzero(session,sizeof(usqld_session));
  session->session_id = session_id;
  usqld_session_store_insert(socket->sessions,session);
  return session;
}

/**
   sets the default packet handler for this socket. 

   returns the previous handler.

 */
usqld_packet_handler  usqld_socket_set_dfl_handler(usqld_socket * socket,
						   usqld_packet_handler handler){
  assert(NULL!=socket);
  usqld_packet_handler oldh;
  
  oldh = socket->default_packet_handler;
  socket->default_packet_handler= handler;
  return oldh;
}

/**
   returns the current default handler for a socket. 
*/
usqld_packet_handler usqld_session_get_dfl_handler(usqld_socket * socket){
  assert(NULL!=socket);
  return socket->default_packet_handler;
}



int usqld_session_get_id(usqld_session* session){
  assert(NULL!=session);
  
  return session->session_id;
}

void * usqld_session_get_data(usqld_session* session){
  assert(NULL!=session);

  return session->ext_data;
}

void * usqld_session_set_data(usqld_session* session, void * data){
  void * old_data;

  assert(NULL!=session);

  old_data = session->ext_data;
  session->ext_data = data;
  return old_data;
}


/**
   sets the destructor for the user data associated with this session
   when a socket is dropped, all sessions and their data will be nuked. 
 */
usqld_session_data_destructor  
usqld_session_set_destructor(usqld_session* session,
			     usqld_session_data_destructor new_dest){
  usqld_session_data_destructor dest;

  assert(NULL!=session);

  dest = session->dest;
  session->dest = new_dest;
  return dest;
}

/**
   
 */
usqld_packet_handler
usqld_session_set_handler(usqld_session* session, 
			  usqld_packet_handler handler){
  usqld_packet_handler old_handler;
  
  assert(NULL!=session);
  old_handler = session->packet_handler;
  session->packet_handler = handler;
  return old_handler;
}

usqld_packet_handler usqld_session_get_handler(usqld_session* session){
  assert(NULL!=session);
  return session->packet_handler;
}

void usqld_free_session(usqld_session* session){
  assert(NULL!=session);
  if(session->dest && session->ext_data)
    session->dest (session->ext_data);
  XDR_free(session);
}


usqld_session_store * usqld_session_store_new(){
  usqld_session_store * store;
  store = XDR_malloc(usqld_session_store);
  bzero(store,sizeof(usqld_session_store));
  return store;
}

void   usqld_session_store_insert(usqld_session_store * store,usqld_session * session){
  usqld_session_store_elem * new_elem;
  assert(NULL!=store);
  assert(NULL!=session);
  
  new_elem = XDR_malloc(usqld_session_store_elem);
  new_elem->session = session;
  new_elem->next = store->top;
  store->top = new_elem;
  
}

usqld_session * usqld_session_store_find(usqld_session_store * store,int session_id){
  usqld_session_store_elem * elem = NULL;
  
  elem = store->top;

  while(NULL!=elem){
    if(usqld_session_get_id(elem->session)==session_id)
      return elem->session;

    elem =elem->next;
  }
  return NULL;
}

usqld_session * usqld_session_store_remove(usqld_session_store * store,int session_id){
  usqld_session_store_elem * prev = store->top,*elem;
  usqld_session * session = NULL;
  if(NULL==prev)
    return NULL;
  
  if(usqld_session_get_id(prev->session)==session_id){
    session = prev->session;
    store->top = prev->next;
    XDR_free(prev);
    
  }else
    while(NULL!= prev->next){
      if(usqld_session_get_id(prev->next->session)==session_id){
	session = prev->next->session;
	elem= prev->next;
	prev->next = prev->next->next;
	XDR_free(elem);
      }
    }
  
  return session;
}


/**
   dispatches an unwrapped packet to its session destination.
 */
int usqld_dispatch_packet(usqld_socket * socket, 
			  unsigned int session_id,
			  usqld_packet * packet){
  usqld_session* session;
  
  if(NULL==(session=usqld_session_store_find(socket->sessions,session_id)) && NULL!=socket->default_packet_handler){
    socket->default_packet_handler(socket,NULL,session_id,packet);
    return 1;
  }else if (NULL!=session->packet_handler){
    session->packet_handler(socket,session,session_id,packet);
    return 1;
  }
  return 0;
}

/**
   a /nice/ asynchrounous pump
 */
void usqld_pump_sock(usqld_socket * sock){
  usqld_envelope *new_packet;
  usqld_packet *content;
  int rv;
  unsigned int session_id;
  
  if(usqld_socket_data_waiting(sock)){
    if(USQLD_OK==(rv = usqld_recv_packet(sock,&new_packet))){
      content = usqld_envelope_get_packet(new_packet);
      session_id = usqld_env_get_session_id(new_packet);
      usqld_dispatch_packet(usqld_envelope_get_packet(new_packet));
      XDR_tree_free(new_packet);
    }else{
      
#ifdef VERBOSE_DEBUG
      fprintf(stderr,"usqld_pump_sock: error %d while receiving\n",rv);
#endif
      //error callback here. 
    }
  }
}


void * usqld_socket_set_data(usqld_socket * socket, 
			     void * data){
  void * data;
  assert(socket!=NULL);
  data = socket->ext_data;
  socket->ext_data = data;
  return data;
}

void * usqld_socket_get_data(usqld_socket * socket){
  assert(socket!=NULL);
  return socket->ext_data;
}

