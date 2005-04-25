
SUBDIRS = po

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir)/gpe -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE \
           -DPACKAGE_LOCALE_DIR=\"@gpecontacts_localedir@\" \
           -DDBUS_API_SUBJECT_TO_CHANGE

bin_PROGRAMS = gpe-contacts

gpe_contacts_LDADD = @DEPS_LIBS@

gpe_contacts_SOURCES = main.c main.h configure.c db.c db.h \
                       editing.c export.c export.h \
                       finddlg.c finddlg.h import-vcard.c import-vcard.h \
                       namedetail.c namedetail.h old-db.c \
                       structure.c structure.h support.c support.h \
                       proto.h

desktopdir = $(datadir)/applications
desktop_in_files = gpe-contacts.desktop.in edit-vcard.desktop.in

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/gpe/pixmaps/default/gpe-contacts

pixmaps_DATA = icons/entry.png \
               icons/export.png \
               icons/frame.png \
               icons/notebook.png

pixmaps_extradir = $(datadir)/gpe/pixmaps/default
pixmaps_extra_DATA = icons/gpe-contacts.png \
                     icons/edit-vcard.png

metadir = $(datadir)/gpe-contacts
meta_DATA = default-layout-bigscreen.xml \
            default-layout.xml \
            well-known-tags

EXTRA_DIST = $(desktop_DATA) \
             $(pixmaps_DATA) \
             $(pixmaps_extra_DATA) \
             $(meta_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in
			 

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

include gpe-dist.am
