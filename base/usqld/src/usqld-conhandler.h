#ifndef USQLD_CONHANDLER_H
#define USQLD_CONHANDLER_H

//allocated struct passed to each new thread on creation
//the thread is responsible for freing this struct.
typedef struct 
{
  int fd; // the incoming client socket fd. 
  usqld_config * config;
}usqld_conhand_init;

void * usqld_conhandler_main(usqld_conhand_init * );


  
 
#endif
