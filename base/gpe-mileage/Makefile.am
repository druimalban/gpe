SUBDIRS = src po

desktopdir = $(datadir)/applications
desktop_in_files = gpe-mileage.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = gpe-mileage.png

EXTRA_DIST = gpe-mileage.desktop.in \
	     gpe-mileage.png \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	rm -f gpe-mileage.desktop
	rm -f po/.intltool-merge-cache

include gpe-dist.am
