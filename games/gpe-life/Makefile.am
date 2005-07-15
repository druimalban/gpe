SUBDIRS = src po

desktopdir = $(datadir)/applications
desktop_in_files = gpe-life.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = gpe-life.png

EXTRA_DIST = gpe-life.desktop.in \
	     gpe-life.png \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	rm -f gpe-life.desktop
	rm -f po/.intltool-merge-cache

include gpe-dist.am
