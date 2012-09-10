SUBDIRS = doc

lib_LTLIBRARIES = libschedule.la

if HAVEAT
DEPS_CFLAGS += -DHAVE_AT
endif

CFLAGS += -Wall

INCLUDES = @SCHEDULE_CFLAGS@ $(DEPS_CFLAGS)  -DPACKAGE_LOCALE_DIR=\"@datadir@/locale/@PACKAGE_NAME@\"

libschedule_la_LIBADD = @SCHEDULE_LIBS@

libschedule_la_SOURCES = atd.c gpe/schedule.h

gpeincludedir = $(includedir)/gpe
gpeinclude_HEADERS = gpe/schedule.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libschedule.pc

include gpe-dist.am
