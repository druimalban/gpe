
INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE

CFLAGS += -Wall

bin_PROGRAMS = gpe-confd

gpe_confd_LDADD = @DEPS_LIBS@

gpe_confd_SOURCES = main.c xsettings-manager.c xsettings-manager.h

xsessiondir = $(sysconfdir)/X11/Xsession.d
xsession_SCRIPTS = gpe-confd.xs

EXTRA_DIST = \
             $(xscripts_SCRIPTS)

install-data-hook:
	cd $(DESTDIR)$(xsessiondir) && mv gpe-confd.xs 70gpe-confd

uninstall-hook:
	$(RM) -f $(DESTDIR)$(xsessiondir)/70gpe-confd

clean-local:
	rm -rf autom4te.cache
	rm -f *.d *~

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar xzf $(top_builddir)/../$(PACKAGE)-$(VERSION).tar.gz
	cp -r $(top_builddir)/debian $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	
include gpe-dist.am
