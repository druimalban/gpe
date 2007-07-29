lib_LTLIBRARIES = libsoundgen.la

AM_CPPFLAGS = @DEPS_CFLAGS@ -DPACKAGE_LOCALE_DIR=\"@datadir@/locale/@PACKAGE_NAME@\" \
              -I$(top_srcdir)/gpe

libsoundgen_la_LIBADD = @DEPS_LIBS@
libsoundgen_la_LDFLAGS = -version-info 0:0:0

libsoundgen_la_SOURCES = soundgen.c

gpeincludedir = $(includedir)/gpe
gpeinclude_HEADERS = gpe/soundgen.h gpe/buzzer.h

# GRC - remove the following lie once we have checked it will cause no problems
include_HEADERS = gpe/soundgen.h gpe/buzzer.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libsoundgen.pc

include gpe-dist.am
