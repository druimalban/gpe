lib_LTLIBRARIES = libtododb.la

INCLUDES = @TODODB_CFLAGS@ -DPACKAGE_LOCALE_DIR=\"@datadir@/locale/@PACKAGE_NAME@\"

libtododb_la_LIBADD = @TODODB_LIBS@

libtododb_la_SOURCES = old-db.c todo-db.c gpe/todo-db.h

gpeincludedir = $(includedir)/gpe
gpeinclude_HEADERS = gpe/todo-db.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtododb.pc

include gpe-dist.am
