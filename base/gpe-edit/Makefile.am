
SUBDIRS = po

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE \
           -DPACKAGE_LOCALE_DIR=\"@gpeedit_localedir@\"

bin_PROGRAMS = gpe-edit

gpe_edit_LDADD = @DEPS_LIBS@

gpe_edit_SOURCES = main.c

desktopdir = $(datadir)/applications
desktop_in_files = gpe-edit.desktop.in

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = gpe-edit.png

helpdir = $(datadir)/doc/gpe
help_DATA = doc/gpe-edit.html

appregdir = $(datadir)/application-registry
appreg_DATA = gpe-edit.applications

mimedir = /etc/mime-handlers
mime_DATA = gpe-edit.mime

EXTRA_DIST = $(desktop_DATA) \
             $(pixmaps_DATA) \
             $(help_DATA) \
             $(appreg_DATA) \
             $(mime_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in
			 

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	rm -f gpe-edit.desktop
	rm -f *.d *~

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar xzf $(top_builddir)/../$(PACKAGE)-$(VERSION).tar.gz
	cp -r $(top_builddir)/debian $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	
include gpe-dist.am
include gpe-tp.am
