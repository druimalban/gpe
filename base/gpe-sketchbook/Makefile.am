
SUBDIRS = src po

desktopdir = $(datadir)/applications
desktop_in_files = gpe-sketchbook.desktop.in

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = gpe-sketchbook.png

pixmapsextradir = $(datadir)/pixmaps/gpe/default/gpe-sketchbook
pixmapsextra_DATA =     \
	pixmaps/brush_large.png \
	pixmaps/brush_medium.png \
	pixmaps/brush_small.png \
	pixmaps/brush_xlarge.png \
	pixmaps/tool_eraser.png \
	pixmaps/tool_pencil.png \
	pixmaps/welcome.png

EXTRA_DIST = $(desktop_DATA) \
	     $(pixmaps_DATA) \
	     $(pixmapsextra_DATA) \
	     intltool-extract.in \
	     intltool-merge.in \
	     intltool-update.in \
	     gpe-sketchbook.desktop.in.in \
	     gpe-sketchbook.desktop.in \
	     po/gpe-sketchbook.pot 

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	rm -f gpe-sketchbook.desktop
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
