bin_PROGRAMS=httplike_test_server httplike_test_client 
	    
#extra_PROGRAMS =usqld_client_test protocol_test \
#		 concurrent_test interrupt_test  
		 
httplike_test_server_SOURCES=httplike_server.c
httplike_test_server_LDADD= -lhttplike -L../httplike/ -lpthread

httplike_test_client_SOURCES=httplike_client.c
httplike_test_client_LDADD= -lhttplike -L../httplike/

#usqld_client_test_SOURCES=usqld-client-test.c
#usqld_client_test_LDADD= -lusqld  -L../src/ 

#concurrent_test_SOURCES=concurrent-test.c
#concurrent_test_LDADD= -lusqld  -L../src/ 

#interrupt_test_SOURCES=interrupt-test.c
#interrupt_test_LDADD= -lusqld  -L../src/ 

#protocol_test_SOURCES = protocol_test.c
#protocol_test_LDADD = -lusqld -L../src/

CFLAGS= -Wall -g -I../src -I../xdr -I../ 
