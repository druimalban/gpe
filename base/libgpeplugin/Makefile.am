SUBDIRS = src examples doc

#libgpeplugindocdir = ${prefix}/doc/libgpeplugin
#libgpeplugindoc_DATA = \
#	README\
#	COPYING\
#	AUTHORS\
#	ChangeLog\
#	INSTALL\
#	NEWS\
#	TODO

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgpeplugin.pc

EXTRA_DIST = $(libgpeplugindoc_DATA) \
	     debian/changelog \
	     debian/control \
	     debian/compat \
	     debian/rules \
	     debian/libgpeplugin0.files \
	     debian/libgpeplugin-dev.files
		


gpeincludedir = $(includedir)/gpe
gpeinclude_HEADERS = include/gpe/gpe-plugin.h


clean-local:
	rm -rf *.d autom4te.cache


deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar zxf ../$(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
