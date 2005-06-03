SUBDIRS = doc
lib_LTLIBRARIES = libgpevtype.la

INCLUDES = $(VTYPE_CFLAGS) -DPACKAGE_LOCALE_DIR=\"@datadir@/locale/@PACKAGE_NAME@\"

libgpevtype_la_LIBADD = @VTYPE_LIBS@

libgpevtype_la_SOURCES = tag-db.c vcard.c vevent.c vtodo.c 

gpeincludedir = $(includedir)/gpe
gpeinclude_HEADERS = gpe/tag-db.h gpe/vevent.h \
                     gpe/vtodo.h gpe/vcard.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgpevtype.pc

EXTRA_DIST = debian/changelog \
             debian/control \
             debian/copyright \
			 debian/compat \
			 debian/rules \
			 debian/libgpevtype-dev.files \
			 debian/libgpevtype0.files

clean-local:
	rm -rf po/.intltool-merge-cache *.d autom4te.cache

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar zxf ../$(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	
DISTCLEANFILES = 

doc:
	make -C doc
	
include gpe-dist.am

.PHONY: doc
