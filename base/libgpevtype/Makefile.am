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

EXTRA_DIST = 

clean-local:
	rm -rf po/.intltool-merge-cache *.d autom4te.cache

DISTCLEANFILES = 

doc:
	make -C doc
	
include gpe-dist.am

.PHONY: doc
