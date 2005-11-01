SUBDIRS = src po

desktopdir = $(datadir)/applications
desktop_in_files = gpe-screenshot.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = gpe-screenshot.png

EXTRA_DIST = gpe-screenshot.desktop.in \
	     gpe-screenshot.png \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	rm -f gpe-screenshot.desktop
	rm -f po/.intltool-merge-cache

include gpe-dist.am
