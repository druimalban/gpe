lib_LTLIBRARIES=libusqld.la
bin_PROGRAMS=usqld 
INCLUDES= -I../xdr/ -I@SQLITEPREFIX@/include
LDFLAGS=-L../xdr/ -L@SQLITEPREFIX@/lib
CFLAGS=-Os -Wall @DBGFLAGS@

usqld_LDADD=-lxdr -lpthread -lsqlite -lusqld

usqld_SOURCES=usqld-server.c usqld-server.h usqld-conhandler.c \
	      usqld-conhandler.h usqld-protocol.h usqld-network.h

libusqld_la_SOURCES=usqld-protocol.h usqld-protocol.c  usql.h \
		usqld-client.c usqld-table.c usqld-printf.c usqld-network.h \
		usqld-network.c

libusqld_la_LIBADD=-lxdr -lpthread

