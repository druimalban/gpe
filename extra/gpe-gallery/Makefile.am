
SUBDIRS = po

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE \
	   -DPACKAGE_LOCALE_DIR=\"@gpegallery_localedir@\"

bin_PROGRAMS = gpe-gallery

gpe_gallery_LDADD = @DEPS_LIBS@

gpe_gallery_SOURCES = main.c image_tools.c

desktopdir = $(datadir)/applications
desktop_in_files = gpe-gallery.desktop.in

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = gpe-gallery.png 

pixmapsextradir = $(datadir)/gpe/pixmaps/default/gallery
pixmapsextra_DATA =	\
		pixmaps/blur.png \
		pixmaps/cursor-pan.xpm \
		pixmaps/cursor-rotate.xpm \
		pixmaps/flip.png \
		pixmaps/fullscreen.png \
		pixmaps/mirror.png \
		pixmaps/pan.png \
		pixmaps/rotate.png \
		pixmaps/sharpen.png \
		pixmaps/slideshow.png \
		pixmaps/zoom_1.png \
		pixmaps/zoom_fit.png \
		pixmaps/zoom_in.png \
		pixmaps/zoom_out.png

appregdir = $(datadir)/application-registry
appreg_DATA = gpe-gallery.applications

mimedir = /etc/mime-handlers
mime_DATA = gpe-gallery.mime

EXTRA_DIST = $(desktop_DATA) \
             $(pixmaps_DATA) \
             $(pixmapsextra_DATA) \
             $(help_DATA) \
             $(appreg_DATA) \
             $(mime_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in
			 

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	rm -f gpe-gallery.desktop
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
