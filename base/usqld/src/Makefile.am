lib_LTLIBRARIES=libusqld.la
bin_PROGRAMS=usqld 
INCLUDES= -I../xdr/
LDFLAGS=-L../xdr/
CFLAGS=-g -Wall 

usqld_LDADD=-lxdr -lpthread -lsqlite -lpopt

usqld_SOURCES=usqld-server.c usqld-conhandler.c usqld-protocol.c 

libusqld_la_SOURCES=usqld-protocol.h usqld-protocol.c usqld-client.c
libusqld_la_LIBADD=-lxdr -lpthread

