
SUBDIRS=examples
lib_LTLIBRARIES=libxdr.la
bin_PROGRAMS=

libxdr_la_SOURCES = xdr-deserialize.c  xdr-schema.c  xdr-serialize.c \
		  xdr-tree.c xdr.h xdr-tree.h xdr-schema.h
LDFLAGS=-L../

CFLAGS= -Os -Wall @DBGFLAGS@