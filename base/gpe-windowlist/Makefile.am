#SUBDIRS = po

bin_PROGRAMS = gpe-windowlist

gpe_windowlist_SOURCES = main.c globals.h

gpe_windowlist_CFLAGS = @DEPS_CFLAGS@ -DPREFIX=\"@prefix@\" -D_GNU_SOURCE -Wall
           
LDADD = @DEPS_LIBS@

desktopdir = $(datadir)/applications
desktop_in_files = gpe-windowlist.desktop.in

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/gpe/pixmaps/default/

pixmaps_DATA = pixmaps/windowlist.png \
	       pixmaps/other-app.png

EXTRA_DIST = gpe-windowlist.desktop.in.in \
             $(pixmaps_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in \
             po/POTFILES.in

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update \
                 po/POTFILES \
                 po/.intltool-merge-cache \
                 apps/gpe-windowlist.desktop

include gpe-dist.am
include gpe-tp.am
