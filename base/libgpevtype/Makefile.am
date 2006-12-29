SUBDIRS = doc

lib_LTLIBRARIES = libgpevtype.la

AM_CPPFLAGS = $(VTYPE_CFLAGS) \
	-DPACKAGE_LOCALE_DIR=\"@datadir@/locale/@PACKAGE_NAME@\"

libgpevtype_la_LIBADD = @VTYPE_LIBS@
libgpevtype_la_LDFLAGS = -version-info 1:0:0

libgpevtype_la_SOURCES = priv.h tag-db.c vcal.c vcard.c vevent.c vtodo.c 

gpeincludedir = $(includedir)/gpe
gpeinclude_HEADERS = gpe/vtype.h gpe/vcal.h gpe/tag-db.h gpe/vevent.h \
                     gpe/vtodo.h gpe/vcard.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgpevtype.pc

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

clean-local:
	rm -rf po/.intltool-merge-cache *.d autom4te.cache

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar zxf ../$(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	cp -r $(top_builddir)/debian $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)

DISTCLEANFILES = 

.PHONY: doc
doc:
	make -C doc

include gpe-dist.am

