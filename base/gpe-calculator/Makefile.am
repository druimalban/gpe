
SUBDIRS = po

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir) \
	   -DPREFIX=\"@prefix@\" -D_GNU_SOURCE \
	   -DPACKAGE_LOCALE_DIR=\"@gpecalculator_localedir@\" \
	   -DPACKAGE_DATA_DIR=\"$(datadir)\"

bin_PROGRAMS = gpe-calculator

gpe_calculator_LDADD = @DEPS_LIBS@

gpe_calculator_SOURCES = main.c \
	calc_basic.c callbacks.c display.c general_functions.c \
	interface.c math_functions.c support.c \
	calc_basic.h callbacks.h display.h galculator.h general_functions.h \
	interface.h math_functions.h support.h

desktopdir = $(datadir)/applications
desktop_in_files = gpe-calculator.desktop.in

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = gpe-calculator.png 

EXTRA_DIST = $(desktop_DATA) \
             $(pixmaps_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in
			 

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	rm -f gpe-calculator.desktop
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
