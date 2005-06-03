SUBDIRS = doc

lib_LTLIBRARIES = libtododb.la

INCLUDES = @TODODB_CFLAGS@ -DPACKAGE_LOCALE_DIR=\"@datadir@/locale/@PACKAGE_NAME@\"

libtododb_la_LIBADD = @TODODB_LIBS@

libtododb_la_SOURCES = old-db.c todo-db.c gpe/todo-db.h

gpeincludedir = $(includedir)/gpe
gpeinclude_HEADERS = gpe/todo-db.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtododb.pc

EXTRA_DIST = libtool-cross.patch \
			 debian/changelog \
			 debian/compat \
			 debian/copyright \	
			 debian/control \
			 debian/rules \
			 debian/libtododb0.files \
			 debian/libtododb-dev.files
 
deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar zxf ../$(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)

include gpe-dist.am
