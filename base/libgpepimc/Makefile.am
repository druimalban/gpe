SUBDIRS = doc po
lib_LTLIBRARIES = libgpepimc.la

if HILDON
PIMC_CFLAGS += -DIS_HILDON
endif

CFLAGS += -Wall

INCLUDES = $(PIMC_CFLAGS) $(HILDON_CFLAGS) -DPACKAGE_LOCALE_DIR=\"@datadir@/locale/@PACKAGE_NAME@\"

libgpepimc_la_LIBADD = @PIMC_LIBS@ @HILDON_LIBS@
libgpepimc_la_LDFLAGS = -version-info 2:0:1

libgpepimc_la_SOURCES = ui.c db.c gpe/pim-categories.h gpe/pim-categories-ui.h \
                        internal.h

gpeincludedir = $(includedir)/gpe
gpeinclude_HEADERS = gpe/pim-categories.h gpe/pim-categories-ui.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgpepimc.pc

EXTRA_DIST = intltool-extract.in \
             intltool-merge.in \
             intltool-update.in \
             po/libgpepimc.pot
 
deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar zxf ../$(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	cp -r $(top_builddir)/debian $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
             
clean-local:
	rm -rf po/.intltool-merge-cache *.d autom4te.cache

DISTCLEANFILES = intltool-extract intltool-merge intltool-update

doc:
	make -C doc

include gpe-dist.am

.PHONY: doc
