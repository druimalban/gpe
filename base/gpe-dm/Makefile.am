
SUBDIRS = po

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE \
           -DPACKAGE_LOCALE_DIR=\"@gpedm_localedir@\"

bin_PROGRAMS = gpe-dm

gpe_dm_LDADD = @DEPS_LIBS@

gpe_dm_SOURCES = gpe-dm.c

initdir = $(sysconfdir)/init.d
init_SCRIPTS = gpe-dm.init

EXTRA_DIST = \
             $(init_SCRIPTS) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in
			 

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

install-data-hook:
	cd $(DESTDIR)$(sysconfdir)/init.d && mv gpe-dm.init gpe-dm

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
