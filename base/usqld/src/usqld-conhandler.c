#ifndef USQLD_CONHANDLER_H
#define USQLD_CONHANDLER_H

//allocated struct passed to each new thread on creation
//the thread is responsible for freing this struct.
typedef struct UsqldConHandInit
{
   int fd; // the incoming client socket fd. 
   
}UsqldConHandInit;

void * usql_conhandler_main(UsqldConHandInit * );


  
 
#endif