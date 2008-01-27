SUBDIRS = src po

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = gpe-mini-browser2.png

desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(pixmaps_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

MAINTAINERCLEANFILES = configure \
		       Makefile.in \
                       depcomp \
		       config.guess \
		       config.sub \
		       ltmain.sh \
		       intltool-extract.in \
		       intltool-update.in \
		       autom4te.cache \
		       intltool-merge.in \
		       missing \
		       aclocal.m4 \
		       install-sh

clean-local:
	rm -f gpe-mini-browser2.desktop
	rm -f po/.intltool-merge-cache 
	
include gpe-dist.am
