
SUBDIRS = po

ENABLE_HILDON = @ENABLE_HILDON@

if HILDON
DEPS_CFLAGS += -DUSE_HILDON
endif

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir)/gpe -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE \
           -DPACKAGE_LOCALE_DIR=\"@gpefilemanager_localedir@\"

bin_PROGRAMS = gpe-filemanager

gpe_filemanager_LDADD = @DEPS_LIBS@

gpe_filemanager_SOURCES = main.c bluetooth.c guitools.c fileops.c \
                          main.h bluetooth.h guitools.h fileops.h

if HILDON

# the hildon desktop file contains some special magic
desktopdir = $(datadir)/applications/hildon
desktop_in_files = hildon/gpe-filemanager.desktop.in

servicedir = $(datadir)/dbus-1/services
service_in_files = hildon/gpe-filemanager.service.in

else

desktopdir = $(datadir)/applications
desktop_in_files = gpe-filemanager.desktop.in

servicedir =
service_in_files =

endif

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
service_DATA = $(service_in_files:.service.in=.service)

pixmapsextradir = $(datadir)/gpe/pixmaps/default/filemanager/document-icons/
pixmapsextra_DATA = \
                   pixmaps/document-icons/directory.png \
                   pixmaps/document-icons/image-png.png \
                   pixmaps/document-icons/executable.png \
                   pixmaps/document-icons/image-xpm.png \
                   pixmaps/document-icons/application-pdf.png \
                   pixmaps/document-icons/text-plain.png \
                   pixmaps/document-icons/application-x-compress.png \
                   pixmaps/document-icons/text-html.png \
                   pixmaps/document-icons/application-x-gzip.png \
                   pixmaps/document-icons/text-x-copying.png \
                   pixmaps/document-icons/application-x-ipk.png \
                   pixmaps/document-icons/text-x-credits.png \
                   pixmaps/document-icons/application-x-ogg.png \
                   pixmaps/document-icons/text-xml.png \
                   pixmaps/document-icons/audio-mpeg.png \
                   pixmaps/document-icons/text-x-python.png \
                   pixmaps/document-icons/audio-x-gsm.png \
                   pixmaps/document-icons/text-x-sql.png \
                   pixmaps/document-icons/audio-x-wav.png \
                   pixmaps/document-icons/video-mpeg.png \
                   pixmaps/document-icons/font-ttf.png \
                   pixmaps/document-icons/video-x-msvideo.png \
                   pixmaps/document-icons/image-gif.png \
                   pixmaps/document-icons/regular.png \
                   pixmaps/document-icons/image-jpeg.png \
                   pixmaps/document-icons/symlink.png


pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = pixmaps/gpe-filemanager.png


EXTRA_DIST =  \
             $(pixmaps_DATA) \
			 $(service_DATA) \
             $(pixmapsextra_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in \
             hildon/gpe-filemanager.desktop.in.in \
             hildon/gpe-filemanager.desktop.in \
             hildon/gpe-filemanager.service.in \
			 gpe-filemanager.desktop.in


DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	rm -f gpe-filemanager.desktop hildon/gpe-filemanager.desktop
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