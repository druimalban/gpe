SUBDIRS = doc

lib_LTLIBRARIES = libeventdb.la

INCLUDES = @EVENTDB_CFLAGS@ -D_XOPEN_SOURCE -DPACKAGE_LOCALE_DIR=\"@datadir@/locale/@PACKAGE_NAME@\"

libeventdb_la_LIBADD = @EVENTDB_LIBS@

libeventdb_la_SOURCES = event-db.c old-db.c gpe/event-db.h

gpeincludedir = $(includedir)/gpe
gpeinclude_HEADERS = gpe/event-db.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libeventdb.pc

include gpe-dist.am
