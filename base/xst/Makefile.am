
INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE

bin_PROGRAMS = xst

xst_LDADD = @DEPS_LIBS@

xst_SOURCES = xst.c

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
