
SUBDIRS = po

if HILDON
DEPS_CFLAGS += -DIS_HILDON
SUBDIRS += hildon
endif

INCLUDES = $(DEPS_CFLAGS) $(HILDON_CFLAGS) $(SERVERDEPS_CFLAGS) -I$(top_srcdir)/gpe -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE -Wall \
           -DPACKAGE_LOCALE_DIR=\"@gpecontacts_localedir@\" \
           -DDBUS_API_SUBJECT_TO_CHANGE

#LDFLAGS = -pg
#INCLUDES = $(INCLUDES) -pg

bin_PROGRAMS = gpe-contacts

gpe_contacts_LDADD = $(DEPS_LIBS) $(HILDON_LIBS) 

gpe_contacts_SOURCES = main.h configure.c \
                       editing.c export.c export.h \
                       finddlg.c finddlg.h import-vcard.c import-vcard.h \
                       namedetail.c namedetail.h \
                       structure.c structure.h support.c support.h \
                       proto.h

if HILDON

desktopdir = 
desktop_in_files = 
                   
gpe_contacts_SOURCES += main-hildon.c

pixmaps_extradir = $(datadir)/pixmaps
pixmaps_extra_DATA =

else

desktopdir = $(datadir)/applications
desktop_in_files = gpe-contacts.desktop.in edit-vcard.desktop.in

gpe_contacts_SOURCES += main.c

pixmaps_extradir = $(datadir)/pixmaps/gpe/default/gpe-contacts
pixmaps_extra_DATA = icons/entry.png \
               icons/export.png \
               icons/frame.png \
               icons/notebook.png
endif

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/pixmaps
metadir = $(datadir)/gpe-contacts

if HILDON

meta_DATA = default-layout.xml \
            well-known-tags

pixmaps_DATA = icons/edit-vcard.png

else

meta_DATA = default-layout-bigscreen.xml \
            default-layout.xml \
            well-known-tags

pixmaps_DATA = icons/gpe-contacts.png \
               icons/edit-vcard.png


endif

EXTRA_DIST = $(pixmaps_DATA) \
             $(pixmaps_extra_DATA) \
	     $(meta_DATA) \
	     $(desktop_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in \
             po/gpe-contacts.pot

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update \
                 gpe-contacts.desktop.in \
                 edit-vcard.desktop.in

clean-local:
	rm -f gpe-contacts.desktop
	rm -f edit-vcard.desktop
	rm -f po/.intltool-merge-cache po/stamp-it
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
