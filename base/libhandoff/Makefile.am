lib_LTLIBRARIES = libhandoff.la

AM_CPPFLAGS = @DEPS_CFLAGS@ -DPACKAGE_LOCALE_DIR=\"@datadir@/locale/@PACKAGE_NAME@\"

libhandoff_la_LIBADD = @DEPS_LIBS@
libhandoff_la_LDFLAGS = -version-info 0:0:0

libhandoff_la_SOURCES = handoff.c handoff.h

include_HEADERS = handoff.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libhandoff.pc

include gpe-dist.am
