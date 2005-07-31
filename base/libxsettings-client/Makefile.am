SUBDIRS = doc

source_h = xsettings-client.h  xsettings-common.h
source_c = xsettings-client.c  xsettings-common.c

libxsincludedir=$(includedir)

lib_LTLIBRARIES      = libXsettings-client.la

libxsinclude_HEADERS= $(source_h)

libXsettings_client_la_SOURCES= $(source_c) $(source_h)

libXsettings_client_la_LIBADD      = @X_LIBS@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxsettings-client.pc

EXTRA_DIST = config.h.in \
             gpe-dist.am \
             gpe-tp.am \
             $(pkgconfig_DATA) 

include gpe-dist.am
include gpe-tp.am
