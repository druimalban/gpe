#ifndef USQLD_PROTOCOL_H 
#define USQLD_PROTOCOL_H


#define USQLD_OK 0
#define USQLD_BUFFER_OVERRUN 1
#define USQLD_PMALLOC 2
#define USQLD_MALLOC 3
#define USQLD_SEND 4
#define USQLD_QUE 5
#define USQLD_RECV 6

typedef struct 
{
  unsigned int  type;
}usqld_pickle;


typedef struct {
   usqld_pickle p;
   size_t database_len;
   char * database;
}usqld_connect_pickle;

typedef   usqld_pickle usqld_disconnect_pickle;

typedef struct {
   usqld_pickle p;
   size_t query_len;
   char * query;
}usqld_query_pickle;

typedef struct{
   usqld_pickle p;
   int errcode;
   size_t errstring_len;
   char * errstring;
} usqld_error_pickle ;


typedef  usqld_pickle usqld_ok_pickle;


typedef  struct {
   usqld_pickle p;
   u_int32_t num_fields;
   u_int32_t *field_lengths;
   char ** field_titles;
}usqld_startrows_pickle;

typedef struct
{
  usqld_pickle p;
  size_t num_fields;
  size_t *field_lengths;
  char ** fields;
} usqld_row_pickle;

typedef  usqld_pickle usqld_eof_pickle;

usqld_pickle * usqld_create_connect(const char * database);
usqld_pickle * usqld_create_disconnect();
usqld_pickle * usqld_create_query(const char * query);
usqld_pickle * usqld_create_error(unsigned int errcode,const char *errstr);
usqld_pickle * usqld_create_ok();
usqld_pickle * usqld_create_start_rows(unsigned int nfields, const char ** fieldtitles);
usqld_pickle * usqld_create_row(unsigned int nfields, const char ** fields);
usqld_pickle * usqld_create_eof();



void uslqd_pickle_free(usqld_pickle*p);


int usqld_send_pickle(int fd,usqld_pickle *p);
int usqld_recv_pickle(int fd, usqld_pickle **p);





   



#endif
