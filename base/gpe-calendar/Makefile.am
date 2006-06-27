
SUBDIRS = po

if HILDON
DEPS_CFLAGS += -DIS_HILDON
endif

INCLUDES = $(DEPS_CFLAGS) $(SERVERDEPS_CFLAGS) -I$(top_srcdir)/gpe -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE -Wall \
           -DPACKAGE_LOCALE_DIR=\"@gpecalendar_localedir@\" \
           -DDBUS_API_SUBJECT_TO_CHANGE

#LDFLAGS = -pg
#INCLUDES = $(INCLUDES) -pg

noinst_LTLIBRARIES = libwidgets.la

libwidgets_la_SOURCES = \
		event-list.c event-list.h \
		calendars-widgets.c calendars-widgets.h \
		util.c util.h

bin_PROGRAMS = gpe-calendar

gpe_calendar_LDADD = $(DEPS_LIBS) libwidgets.la

gpe_calendar_SOURCES = main.c						\
		       globals.h util.h					\
                       day_view.c day_view.h				\
                       week_view.c week_view.h				\
		       event-menu.c event-menu.h			\
		       event-ui.c event-ui.h				\
		       month_view.c month_view.h			\
		       day_popup.c day_popup.h				\
		       xsettings.c					\
		       gtkdatesel.c gtkdatesel.h			\
		       import-vcal.c import-vcal.h			\
		       export-vcal.c export-vcal.h			\
		       event-cal.c event-cal.h				\
		       view.c view.h					\
		       alarm-dialog.c alarm-dialog.h			\
		       calendar-edit-dialog.c calendar-edit-dialog.h	\
		       calendar-delete-dialog.c				\
		       calendar-delete-dialog.h				\
		       calendars-dialog.c calendars-dialog.h		\
		       calendar-update.c calendar-update.h

if HILDON

desktopdir = $(datadir)/applications/hildon
desktop_in_files = hildon/gpe-calendar.desktop.in

servicedir = $(datadir)/dbus-1/services
service_in_files = hildon/com.nokia.gpe-calendar.service.in

hildonhomelibdir = /usr/lib/hildon-home

hildonhomelib_LTLIBRARIES = libgpe-calendar-applet.la
libgpe_calendar_applet_la_SOURCES = hildon-applet.c inotify-syscalls.h
libgpe_calendar_applet_la_LIBADD = $(DEPS_LIBS) libwidgets.la

hildonhomedir = $(datadir)/applications/hildon-home
hildonhome_in_files = hildon/gpe-calendar-home.desktop.in

else

desktopdir = $(datadir)/applications
desktop_in_files = gpe-calendar.desktop.in

servicedir =
service_in_files =

hildonhomedirlib =
hildonhomedir =

endif

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
service_DATA = $(service_in_files:.service.in=.service)
hildonhome_DATA = $(hildonhome_in_files:.desktop.in=.desktop)

pixmapsdir = $(datadir)/pixmaps

pixmaps_DATA = pixmaps/gpe-calendar.png

EXTRA_DIST = $(service_DATA) \
             $(pixmaps_DATA) \
             gpe-calendar.desktop.in.in \
             hildon/com.nokia.gpe-calendar.service.in \
             hildon/gpe-calendar.desktop.in.in \
	     hildon/gpe-calendar-home.desktop.in \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in \
			 debian/rules \
			 debian/links \
			 debian/control \
			 debian/changelog \
			 debian/compat 


DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update \
                 gpe-calendar.desktop \
		 gpe-calendar-home.desktop

clean-local:
	rm -f gpe-contacts.desktop
	rm -f edit-vcard.desktop
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
