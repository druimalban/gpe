#ifndef USQLD_NETWORK_H
#define USQLD_NETWORK_H
/**
   Specifies interface to abstract sessioned network layer 
 */


/**
 * All usqld packets are just XDR document trees
 */
typedef XDR_tree usqld_packet;


/**
 * A destructor for anonymous bound data. 
 */
typedef void (*usqld_session_data_destructor)(void *data);

typedef struct t_usqld_socket usqld_socket;

typedef struct t_usqld_session usqld_session;


/**
 * A packet handler function takes a socket, a session object (if
 */

typedef void (*usqld_packet_handler)(usqld_socket *sock,usqld_session * session,int session_id,usqld_packet *packet);


typedef struct t_usqld_session_store_elem
{
   usqld_session * session;
   struct t_usqld_session_store_elem * next;
}usqld_session_store_elem;

/**
 * stack as top first access is most likely */
typedef struct t_usqld_session_store
{
   usqld_session_store_elem * top;
}usqld_session_store;




/**
 * a top-level fd connection, with sessions as children */
struct t_usqld_socket{
  /**Counter for uniqueness of session IDs on this connection, always
     starts at 1 (0 is not a session id) */
  unsigned int next_session_id;
  /**Packets which do not have a session are dispatched here*/
  usqld_packet_handler default_packet_handler;
  /**a stack of our sessions*/
  usqld_session_store *  sessions;
  /**the hole we dig ourselves deeper into*/
  int fd;
  void * ext_data;
};


/**
 * As selects can be called during other selects and there is no
 * guarantee at the database that behaviour will necesarily be stacked
 * (although it is) we preserve some state about unfinished
 * transactions */
 struct t_usqld_session
{
   unsigned int session_id;
   void * ext_data;
   usqld_session_data_destructor dest;
   usqld_packet_handler packet_handler;
};





/*
 * physical network access functionality
 */
int usqld_recv_packet(usqld_socket *,usqld_envelope ** packet);
int usqld_send_packet(usqld_socket *,usqld_envelope* packet);
unsigned int usqld_socket_data_waiting(usqld_socket* socket);

int usqld_session_send_packet(usqld_socket * socket,
			      usqld_session * session, 
			      usqld_packet * packet_in);


void usqld_pump_sock(usqld_socket * sock);


usqld_socket * usqld_socket_new_from_fd(int fd);

usqld_session* usqld_socket_new_session(usqld_socket *socket);

usqld_session* usqld_socket_establish_session(usqld_socket *socket,
					      unsigned int session_id);
usqld_packet_handler 
usqld_socket_set_dfl_handler(usqld_socket * , 
			     usqld_packet_handler handler);
void *
usqld_socket_set_data(usqld_socket * , 
		      void * data);
void *
usqld_socket_get_data(usqld_socket * );

usqld_packet_handler usqld_session_get_dfl_handler(usqld_socket * sock);


int usqld_session_get_id(usqld_session*);
void * usqld_session_get_data(usqld_session* session);
void * usqld_session_set_data(usqld_session* session, void * data);
usqld_session_data_destructor  
usqld_session_set_destructor(usqld_session* session,
			     usqld_session_data_destructor dest);


usqld_packet_handler usqld_session_set_handler(usqld_session* session, 
					       usqld_packet_handler handler);
usqld_packet_handler usqld_session_get_handler(usqld_session* session);
void usqld_free_session(usqld_session* session);

usqld_session_store * usqld_session_store_new();
void   usqld_session_store_insert(usqld_session_store * store,
				  usqld_session * session);
usqld_session * usqld_session_store_find(usqld_session_store * store,
					 int session_id);
usqld_session * usqld_session_store_remove(usqld_session_store * store,
					   int session_id);


#define  usqld_get_packet_type(packet) \
 XDR_t_get_union_disc(packet);

#define  usqld_env_get_session_id(packet) \
XDR_t_get_uint(XDR_TREE_SIMPLE(XDR_t_get_comp_elem(XDR_TREE_COMPOUND((usqld_env_packet*)packet),0)));


#define  usqld_envelope_get_packet(packet) \
 XDR_t_get_union_disc(XDR_t_get_comp_elem(XDR_TREE_COMPOUND((usqld_env_packet*)packet),1));


#endif
