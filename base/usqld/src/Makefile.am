lib_LTLIBRARIES=libusqld.la
bin_PROGRAMS=usqld usqld_client_test
INCLUDES= -I../xdr/
LDFLAGS=-L../xdr/
CFLAGS=-g

usqld_LDADD=-lxdr -lpthread -lsqlite -lefence
usqld_client_test_SOURCES=usqld-client-test.c
usqld_client_test_LDADD=-lusqld -lxdr
usqld_SOURCES=usqld-server.c usqld-conhandler.c usqld-protocol.c 

libusqld_la_SOURCES=usqld-protocol.h usqld-protocol.c usqld-client.c
libusqld_la_LIBADD=-lxdr
