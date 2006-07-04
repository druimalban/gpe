
SUBDIRS = doc

#moduledir = @GTK_MODULES_DIR@

# module_LTLIBRARIES = libcontactsdb.la

lib_LTLIBRARIES = libcontactsdb.la

INCLUDES = @CONTACTSDB_CFLAGS@
LDADD = @CONTACTSDB_LIBS@

libcontactsdb_la_LIBADD = @CONTACTSDB_LIBS@
libcontactsdb_la_SOURCES = contacts-db.c gpe/contacts-db.h old-db.c

gpeincludedir = $(includedir)/gpe
gpeinclude_HEADERS = gpe/contacts-db.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libcontactsdb.pc

include gpe-dist.am
include gpe-tp.am
