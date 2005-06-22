
SUBDIRS = po

if HILDON
DEPS_CFLAGS += -DIS_HILDON
endif

INCLUDES = $(DEPS_CFLAGS) $(SERVERDEPS_CFLAGS) -I$(top_srcdir)/gpe -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE -Wall \
           -DPACKAGE_LOCALE_DIR=\"@gpecontacts_localedir@\" \
           -DDBUS_API_SUBJECT_TO_CHANGE

#LDFLAGS = -pg
#INCLUDES = $(INCLUDES) -pg

bin_PROGRAMS = gpe-contacts

gpe_contacts_LDADD = $(DEPS_LIBS)

gpe_contacts_SOURCES = main.h configure.c db.c db.h \
                       editing.c export.c export.h \
                       finddlg.c finddlg.h import-vcard.c import-vcard.h \
                       namedetail.c namedetail.h old-db.c \
                       structure.c structure.h support.c support.h \
                       proto.h

if HILDON

desktopdir = $(datadir)/applications/hildon
desktop_in_files = hildon/gpe-contacts.desktop.in

servicedir = $(libdir)/dbus-1.0/services
service_in_files = hildon/org.gpe.gpe-contacts.service.in
                   
gpe_contacts_SOURCES += main-hildon.c pimc-ui.c pimc-ui.h

pixmaps_extradir = $(datadir)/pixmaps
pixmaps_extra_DATA =

else

desktopdir = $(datadir)/applications
desktop_in_files = gpe-contacts.desktop.in edit-vcard.desktop.in

servicedir =
service_in_files =

gpe_contacts_SOURCES += main.c

pixmaps_extradir = $(datadir)/gpe/pixmaps/default
pixmaps_extra_DATA = icons/entry.png \
               icons/export.png \
               icons/frame.png \
               icons/notebook.png
endif

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
service_DATA = $(service_in_files:.service.in=.service)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/pixmaps

pixmaps_extra_DATA += icons/gpe-contacts.png \
                     icons/edit-vcard.png

metadir = $(datadir)/gpe-contacts

if HILDON

meta_DATA = hildon/default-layout-bigscreen.xml \
            default-layout.xml \
            well-known-tags

# we need a 26x26 icon here 
pixmaps_DATA = hildon/gpe-contacts.png \
               icons/edit-vcard.png

else

meta_DATA = default-layout-bigscreen.xml \
            default-layout.xml \
            well-known-tags

pixmaps_DATA = icons/gpe-contacts.png \
               icons/edit-vcard.png


endif

EXTRA_DIST = $(service_DATA) \
             $(pixmaps_DATA) \
             $(pixmaps_extra_DATA) \
             $(meta_DATA) \
             hildon/default-layout-bigscreen.xml \
             hildon/org.gpe.gpe-contacts.service.in \
             hildon/gpe-contacts.desktop.in.in \
             hildon/gpe-contacts.png \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in \
             debian/changelog \
             debian/compat \
             debian/copyright \
             debian/control \
             debian/rules \
             debian/dirs \
             debian/links
 

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update \
                 gpe-contacts.desktop.in \
                 edit-vcard.desktop.in

clean-local:
	rm -f gpe-contacts.desktop
	rm -f edit-vcard.desktop
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
