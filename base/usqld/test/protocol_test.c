#include <stdio.h> 
#include <sys/types.h>
#include <sys/socket.h> 
#include "usqld-protocol.h"
#include "xdr.h"
#include <assert.h>

XDR_schema * protocol;

void sender(int fd,int d)
{
   
   XDR_tree * packet = NULL;
   

   while(d++<1000)
     {
	int i =0;
	switch(random()%6)
	  {
	   case 0:
	       {
		  
		  XDR_tree * e[2];
	       
		  e[0] = XDR_tree_new_string("FOO");
		  e[1] = XDR_tree_new_string("BAR");
		  
		  packet = XDR_tree_new_union(PICKLE_CONNECT,
					      XDR_tree_new_struct(2,e));
		  sender(fd,d);
	       }
	     break;
	   case 1:
	       {
		  packet = XDR_tree_new_union(PICKLE_QUERY,
					      XDR_tree_new_string("GET ME TO A NUNNARY"));
		  
	       }
	     break;
	     
	   case 2:
	       {
		  packet = usqld_error_packet(69,"there is no error");
	       }
	     break;
	   case 3: 
	       {
		  XDR_tree * e[5];
		  e[0]= XDR_tree_new_string("DOH");
		  e[1]= XDR_tree_new_string("Ray");
		  e[2]= XDR_tree_new_string("Meee");
		  e[3]= XDR_tree_new_string("Farr");
		  e[4]= XDR_tree_new_string("Soo");
		  
		  packet = XDR_tree_new_union(PICKLE_STARTROWS,
					      XDR_tree_new_var_array(5,e));
		  
	       }
	     
	     
	   case 4:
	       {
		  XDR_tree * e[5];
		  e[0]= XDR_tree_new_string("DOH");
		  e[1]= XDR_tree_new_string("Ray");
		  e[2]= XDR_tree_new_string("Meee");
		  e[3]= XDR_tree_new_string("Farr");
		  e[4]= XDR_tree_new_string("Soo");
		  
		  packet = XDR_tree_new_union(PICKLE_ROW,
					      XDR_tree_new_var_array(5,e));
		  
	       }
	     
	     break;
	     
	     
	   case 6:
	       {
		  packet =  usqld_ok_packet();
	       }
	  }
	
	
	
	if(USQLD_OK!=usqld_send_packet(fd,packet))
	  break;
     }   
}

void receiver(int fd)
{
   XDR_tree * packet = NULL;
   
   while(USQLD_OK == usqld_recv_packet(fd,&packet))
     {
	
     }
   
     
     
   
}


int main (int argc, char ** argv) 
{
   int sv[2]
     ;
   
   protocol = usqld_get_protocol();
   assert(protocol!=NULL);
   
   if(0==socketpair(PF_LOCAL,SOCK_STREAM,0,sv))
     {
	if(fork()==0)
	   {
	      sender(sv[0],0);
	   }else
	   {
	      receiver(sv[1]);
	   }
      }
      
   }
