source_h = xsettings-common.h
source_c = xsettings-common.c

INCLUDES = $(X_CFLAGS)

libxsincludedir = $(includedir)

lib_LTLIBRARIES      = libXsettings.la

libxsinclude_HEADERS = $(source_h)

libXsettings_la_SOURCES = $(source_c) $(source_h)

libXsettings_la_LIBADD      = @X_LIBS@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxsettings.pc

EXTRA_DIST = config.h.in \
             gpe-dist.am \
             gpe-tp.am \
             libxsettings.pc.in

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar xzf $(top_builddir)/../$(PACKAGE)-$(VERSION).tar.gz
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	
include gpe-dist.am
