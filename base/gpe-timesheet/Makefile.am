SUBDIRS = po

if HILDON
CFLAGS += -DIS_HILDON
endif

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir)/gpe -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE \
           -DPACKAGE_LOCALE_DIR=\"@gpetimesheet_localedir@\"

bin_PROGRAMS = gpe-timesheet

gpe_timesheet_LDADD = @DEPS_LIBS@ -lsqlite

gpe_timesheet_SOURCES = journal.c journal.h main.c main.h sql.c sql.h ui.c ui.h

if HILDON

# the hildon desktop fiel contains some special magic
desktopdir = $(datadir)/applications/hildon
desktop_in_files = hildon/gpe-timesheet.desktop.in

servicedir = $(datadir)/dbus-1/services
service_in_files = hildon/com.nokia.gpe-timesheet.service.in

# we need a 26x26 icon here
pixmaps_extra_DATA = hildon/gpe-timesheet.png

else

desktopdir = $(datadir)/applications
desktop_in_files = gpe-timesheet.desktop.in

servicedir =
service_in_files =

pixmaps_extra_DATA = pixmaps/gpe-timesheet.png

endif

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
service_DATA = $(service_in_files:.service.in=.service)

pixmapsdir = $(datadir)/gpe-timesheet

#pixmaps_DATA = pixmaps/clock.png \
#               pixmaps/stop-clock.png \
#               pixmaps/new.png \
#               pixmaps/delete.png \
#               pixmaps/journal.png

pixmaps_extradir = $(datadir)/pixmaps


EXTRA_DIST = $(desktop_DATA) \
             $(service_DATA) \
             $(pixmaps_extra_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in \
             debian/changelog \
             debian/compat \
             debian/copyright \
             debian/control \
             debian/rules \
             debian/docs \
             debian/dirs \
             gpe-timesheet.desktop.in.in \
             hildon/gpe-timesheet.desktop.in.in \
             hildon/com.nokia.gpe-timesheet.service.in \
             hildon/gpe-timesheet.png
			 

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
