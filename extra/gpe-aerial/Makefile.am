
SUBDIRS = po

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE \
           -DPACKAGE_LOCALE_DIR=\"@gpeaerial_localedir@\"

bin_PROGRAMS = gpe-aerial

gpe_aerial_LDADD = @DEPS_LIBS@

gpe_aerial_SOURCES = main.c netinfodb.c netedit.c
noinst_HEADERS = main.h netedit.h netinfodb.h prismstumbler.h

desktopdir = $(datadir)/applications
desktop_in_files = gpe-aerial.desktop.in

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/pixmaps

pixmaps_DATA = pixmaps/scan-on.png \
               pixmaps/scan-off.png \
               pixmaps/gpe-aerial.png


EXTRA_DIST = $(desktop_DATA) \
             $(pixmaps_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in
			 

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	rm -f gpe-aerial.desktop
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
