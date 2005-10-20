
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

bin_PROGRAMS = gpe-calendar

gpe_calendar_LDADD = $(DEPS_LIBS)

gpe_calendar_SOURCES = globals.h main.c \
                       day_view.c day_view.h \
                       week_view.c week_view.h \
		       day_render.c day_render.h \
 					   event-ui.c event-ui.h \
					   month_view.c month_view.h \
  					   day_popup.c day_popup.h \
					   xsettings.c \
					   gtkdatesel.c gtkdatesel.h \
					   import-vcal.c import-vcal.h

if HILDON

desktopdir = $(datadir)/applications/hildon
desktop_in_files = hildon/gpe-calendar.desktop.in

servicedir = $(libdir)/dbus-1.0/services
service_in_files = hildon/com.nokia.gpe-calendar.service.in

else

desktopdir = $(datadir)/applications
desktop_in_files = gpe-calendar.desktop.in

servicedir =
service_in_files =

endif

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
service_DATA = $(service_in_files:.service.in=.service)

pixmapsdir = $(datadir)/pixmaps

pixmaps_DATA = pixmaps/gpe-calendar.png

EXTRA_DIST = $(service_DATA) \
             $(pixmaps_DATA) \
             gpe-calendar.desktop.in.in \
             hildon/com.nokia.gpe-calendar.service.in \
             hildon/gpe-calendar.desktop.in.in \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in
 

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update \
                 gpe-calendar.desktop

clean-local:
	rm -f gpe-contacts.desktop
	rm -f edit-vcard.desktop
	rm -rf autom4te.cache
	rm -f *.d *~

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar xzf $(top_builddir)/../$(PACKAGE)-$(VERSION).tar.gz
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	
include gpe-dist.am
include gpe-tp.am
