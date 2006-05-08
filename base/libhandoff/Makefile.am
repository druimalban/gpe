lib_LTLIBRARIES = libhandoff.la

AM_CPPFLAGS = @HANDOFF_CFLAGS@ $(DEPS_CFLAGS)  -DPACKAGE_LOCALE_DIR=\"@datadir@/locale/@PACKAGE_NAME@\"

libhandoff_la_LIBADD = @HANDOFF_LIBS@
libhandoff_la_LDFLAGS = -version-info 0:0:0

libhandoff_la_SOURCES = handoff.c handoff.h

include_HEADERS = handoff.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libhandoff.pc

include gpe-dist.am
