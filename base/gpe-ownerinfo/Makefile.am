
SUBDIRS = po

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE \
	   -DPACKAGE_LOCALE_DIR=\"@gpeownerinfo_localedir@\" \
	   -DENABLE_NLS

bin_PROGRAMS = gpe-ownerinfo

gpe_ownerinfo_LDADD = @DEPS_LIBS@
gpe_ownerinfo_SOURCES = main.c gpe-ownerinfo.c

lib_LIBRARIES=libgpe-ownerinfo.a
libgpe_ownerinfo_a_SOURCES=gpe-ownerinfo.c


gpedir = $(sysconfdir)/gpe

include_HEADERS = gpe-ownerinfo.h

EXTRA_DIST = intltool-extract.in \
             intltool-merge.in \
             intltool-update.in
			 

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	rm -f po/.intltool-merge-cache
	rm -rf autom4te.cache
	rm -f *.d *~

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar xzf $(top_builddir)/../$(PACKAGE)-$(VERSION).tar.gz
	cp -r $(top_builddir)/debian $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	
include gpe-dist.am
include gpe-tp.am
