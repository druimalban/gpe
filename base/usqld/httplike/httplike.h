#ifndef HTTPLIKE_H
#define HTTPLIKE_H
struct httplike_socket;
struct httplike_header;
struct httplike_packet;

/*this is an extrememly cheap http-like protocol implementation, it
  only deals with reception and parsing and is probably wrong in more
  ways than one.*/

#define HTTPLIKE_HEADER_CONTENT_LEN "CONTENT-LENGTH"

typedef void (*httplike_message_func)(struct httplike_socket * sock,struct httplike_packet * packet);

typedef void (*httplike_timeout_func)(struct httplike_socket * sock);

typedef void (*httplike_error_func)(struct httplike_socket * sock,
				   int errcode,const  char * msg);


typedef void (*httplike_data_destructor_func)(struct httplike_socket * sock,
					      void * data);

#define HTTPLIKE_MAX_HEADERS 32

#define HTTPLIKE_ERR_NOMEM 501
#define HTTPLIKE_ERR_MALF_HEADER 502
#define HTTPLIKE_ERR_DISCONNECT 503
#define HTTPLIKE_ERR_TOOMANY_HEADERS 601

#define HTTPLIKE_NET_BUF_LEN 1024

/// the socket state
typedef enum {
  HTTPLIKE_STATE_IDLE,
  HTTPLIKE_STATE_HEADERS,
  HTTPLIKE_STATE_DATA,
  HTTPLIKE_STATE_CLOSED
} httplike_socket_state;

#define HTTPLIKE_NUM_ISTATES 14

//The highest valid transitionalble state.
#define HTPL_IS_MAX_T HTPL_IS_HVAL
///the parser states

typedef enum {
  HTPL_IS_INIT = 0,
  HTPL_IS_OPER,
  HTPL_IS_WTS1,
  HTPL_IS_OPND,
  HTPL_IS_WTS2,
  HTPL_IS_VERS,
  HTPL_IS_H_NL,
  HTPL_IS_WTS3,
  HTPL_IS_HNAM,
  HTPL_IS_COLO,
  HTPL_IS_WTS4,
  HTPL_IS_HVAL,
  HTPL_IS_DATA,
  HTPL_IS_ERRO,
}httplike_internal_state;

#define HTTPLIKE_NUM_GUARDS 5
typedef enum {
  HTTPLIKE_TRIGGER_NONWSPACE,
  HTTPLIKE_TRIGGER_NEWLINE,
  HTTPLIKE_TRIGGER_COLON,
  HTTPLIKE_TRIGGER_WHITESPACE,
} httplike_guard;

typedef struct httplike_trans{
  httplike_internal_state next;
  char chomp;
}httplike_trans;


typedef struct httplike_header{
  char * h_name;
  char * h_val;
}httplike_header;


#define HEADER_BUF_INIT 1024
#define HEADER_BUF_INC 128

typedef struct  httplike_packet {
  unsigned int nheader;
  httplike_header headers[HTTPLIKE_MAX_HEADERS];
  //  unsigned int headers_len;
  char * operation;
  char * operand;
  char * version;
  char * content;
  size_t content_len;
} httplike_packet;



typedef struct httplike_parse_context{
  httplike_internal_state istate;
  char * parsebuf;
  size_t parsebuf_size;
  size_t c_term_len;  // the current ongoing length of the buffer
  httplike_packet packet;
} httplike_parse_context;

typedef struct httplike_socket {
  int fd;
  int timeout;

  void * data;

  httplike_socket_state state;

  httplike_parse_context pc;

  httplike_message_func on_dispatch;
  httplike_timeout_func on_timeout;
  httplike_error_func on_error;
  httplike_data_destructor_func dest_data;
}httplike_socket;


/**
 * sets the packet timeout on a socket, the timeout is called if a
 * socket remains in a busy state with no activity.
 */
int httplike_socket_set_timeout(httplike_socket * sock, int timeout);

/**
 * gets the packet timeout on a socket, the timeout is called if a
 * socket remains in a busy state with no activity.
 */
int httplike_socket_get_timeout(httplike_socket * sock);

/**
 * sets the message callback for a socket, this is called when a
 * complete HTTPlike message is receieved.
 */
httplike_message_func  httplike_socket_set_message_func(httplike_socket * sock,
							httplike_message_func func);
/**
 * gets the message callback for a socket, this is called when a
 * complete HTTPlike message is receieved.
 */
httplike_message_func  httplike_socket_get_message_func(httplike_socket * sock);

/**
 * sets the timeout callback for a socket, this is called when a
 * 
 */
httplike_timeout_func  httplike_socket_set_timeout_func(httplike_socket * sock,
							httplike_timeout_func func);

httplike_timeout_func  httplike_socket_get_timeout_func(httplike_socket * sock);


httplike_error_func  httplike_socket_set_error_func(httplike_socket * sock,
							httplike_error_func func);

httplike_error_func  httplike_socket_get_error_func(httplike_socket * sock);


void * httplike_socket_set_data(httplike_socket * sock, void * data);
void * httplike_socket_get_data(httplike_socket * sock);

httplike_data_destructor_func  httplike_socket_get_data_destructor(httplike_socket * sock);
httplike_data_destructor_func  httplike_socket_set_data_destructor(httplike_socket * sock,httplike_data_destructor_func dest);

httplike_socket* httplike_new_socket(int fd);

void httplike_socket_destroy(httplike_socket * sock);

/**
 * pumps a socket, if there is any data. 
 */
void httplike_pump_socket(httplike_socket *sock);

/**
 */
int  httplike_socket_busy(httplike_socket *sock);

/**
 * adds a header to a packet copies the header fields duplicate
 * headers will be replaced, and Content-length headers are not
 * allowed.
 */
int httplike_packet_add_header(httplike_packet * packet,
			       const char * h_name,
			       const char * h_val);
/**
 * returns a header iff found or NULL
 */
char * httplike_packet_get_header(httplike_packet * packet,
				  const char * h_name);

void httplike_packet_free_headers(httplike_packet *packet);

int httplike_socket_send_packet(httplike_socket *sock, httplike_packet *packet);
void httplike_dump_packet(httplike_packet * packet);
#endif
