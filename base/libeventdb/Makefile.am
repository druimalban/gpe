SUBDIRS = doc

lib_LTLIBRARIES = libeventdb.la

INCLUDES = @EVENTDB_CFLAGS@ -D_XOPEN_SOURCE -DPACKAGE_LOCALE_DIR=\"@datadir@/locale/@PACKAGE_NAME@\"

libeventdb_la_LDFLAGS = @EVENTDB_LIBS@ -version-info 0:1:0

libeventdb_la_SOURCES = event-db.c gpe/event-db.h

gpeincludedir = $(includedir)/gpe
gpeinclude_HEADERS = gpe/event-db.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libeventdb.pc

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

include gpe-dist.am
