bin_PROGRAMS=sqlite_test usqld_client_test protocol_test concurrent_test

sqlite_test_SOURCES=sqlite-test.c

sqlite_test_LDADD=-lsqlite

usqld_client_test_SOURCES=usqld-client-test.c

usqld_client_test_LDADD= -lusqld  -L../src/ 

concurrent_test_SOURCES=concurrent-test.c
concurrent_test_LDADD= -lusqld  -L../src/ 

protocol_test_SOURCES = protocol_test.c
protocol_test_LDADD = -lusqld -L../src/

CFLAGS=-Os -I../src -I../xdr
