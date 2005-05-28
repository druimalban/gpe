SUBDIRS = 

lib_LTLIBRARIES = libschedule.la

INCLUDES = @SCHEDULE_CFLAGS@ -DPACKAGE_LOCALE_DIR=\"@datadir@/locale/@PACKAGE_NAME@\"

libschedule_la_LIBADD = @SCHEDULE_LIBS@

libschedule_la_SOURCES = atd.c gpe/schedule.h

gpeincludedir = $(includedir)/gpe
gpeinclude_HEADERS = gpe/schedule.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libschedule.pc

include gpe-dist.am
