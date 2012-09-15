CFLAGS += -Wall

SUBDIRS = po systray plugins

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir) -DSTATIC_PLUGINS\
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE -Wall \
           -DPACKAGE_LOCALE_DIR=\"@gpefbpanel_localedir@\"

#LDFLAGS = -pg
#INCLUDES = $(INCLUDES) -pg

bin_PROGRAMS = gpe-fbpanel 
##configurator

gpe_fbpanel_LDADD = $(DEPS_LIBS) plugins/.libs/libplugins.a systray/.libs/libsystray.a
gpe_fbpanel_DEPENDENCIES = plugins/libplugins.la systray/libsystray.la
gpe_fbpanel_SOURCES = panel.c misc.c plugin.c gtkbar.c bg.c gtkbgbox.c ev.c configurator.c 

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = 
EXTRA_DIST = $(pixmaps_DATA) \
             po/gpe-fbpanel.pot

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update
clean-local:
	rm -f *.d *~

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar xzf $(top_builddir)/../$(PACKAGE)-$(VERSION).tar.gz
	cp -r $(top_builddir)/debian $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	
include gpe-dist.am
include gpe-tp.am
