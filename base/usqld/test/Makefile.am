EXTRA_PROGRAMS=usqld_client_test protocol_test \
		 concurrent_test interrupt_test


usqld_client_test_SOURCES=usqld-client-test.c
usqld_client_test_LDADD= -lusqld  -L../src/ 

concurrent_test_SOURCES=concurrent-test.c
concurrent_test_LDADD= -lusqld  -L../src/ 

interrupt_test_SOURCES=interrupt-test.c
interrupt_test_LDADD= -lusqld  -L../src/ 

protocol_test_SOURCES = protocol_test.c
protocol_test_LDADD = -lusqld -L../src/

CFLAGS=-pg -I../src -I../xdr
