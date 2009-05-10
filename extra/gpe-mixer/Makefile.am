
SUBDIRS = po

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE -DUSE_GPE \ 
	   -DPACKAGE_LOCALE_DIR=\"@gpemixer_localedir@\"

bin_PROGRAMS = gpe-mixer

gpe_mixer_LDADD = @DEPS_LIBS@

gpe_mixer_SOURCES = gpe-mixer.c

desktopdir = $(datadir)/applications
desktop_in_files = gpe-mixer.desktop.in

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = gpe-mixer.png 

pixmapsextradir = $(datadir)/gpe-mixer
pixmapsextra_DATA =	\
		pixmaps/alarm.png 

EXTRA_DIST = $(desktop_DATA) \
             $(pixmaps_DATA) \
             $(pixmapsextra_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in
			 

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	rm -f gpe-mixer.desktop
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
