SUBDIRS = po

if HILDON
CFLAGS += -DIS_HILDON
SUBDIRS += hildon
endif

CFLAGS += -Wall

INCLUDES = $(DEPS_CFLAGS) $(HILDON_CFLAGS) -I$(top_srcdir)/gpe -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE \
           -DPACKAGE_LOCALE_DIR=\"@gpetimesheet_localedir@\"

bin_PROGRAMS = gpe-timesheet

gpe_timesheet_LDADD = @DEPS_LIBS@ @HILDON_LIBS@ -lsqlite

gpe_timesheet_SOURCES = journal.c journal.h main.c main.h sql.c sql.h ui.c ui.h

if HILDON

desktopdir = 
desktop_in_files = 

pixmaps_extra_DATA = 

else

desktopdir = $(datadir)/applications
desktop_in_files = gpe-timesheet.desktop.in

pixmaps_extra_DATA = pixmaps/gpe-timesheet.png

endif

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/gpe-timesheet

#pixmaps_DATA = pixmaps/clock.png \
#               pixmaps/stop-clock.png \
#               pixmaps/new.png \
#               pixmaps/delete.png \
#               pixmaps/journal.png

pixmaps_extradir = $(datadir)/pixmaps


EXTRA_DIST = gpe-timesheet.desktop.in  \
             $(pixmaps_extra_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in \
             gpe-timesheet.desktop.in.in \
             po/gpe-timesheet.pot
			 

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	rm -f gpe-timesheet.desktop hildon/gpe-timesheet.desktop
	rm -f po/.intltool-merge-cache 
	rm -rf autom4te.cache
	rm -f *.d *~

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar xzf $(top_builddir)/../$(PACKAGE)-$(VERSION).tar.gz
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)

include gpe-dist.am
include gpe-tp.am
