SUBDIRS = po

bin_PROGRAMS = gpe-mini-browser

gpe_mini_browser_SOURCES = interface-calls.c loading-calls.c gpe-mini-browser.h misc-calls.c settings.c

CFLAGS = -Os -Wall -g -pedantic -std=c99

if HILDON
gpe_mini_browser_SOURCES += hildon-mini-browser.c
desktop_in_files = hildon/gpe-mini-browser.desktop.in 
CFLAGS += -DHILDON
else
gpe_mini_browser_SOURCES += gpe-mini-browser.c
desktop_in_files = gpe-mini-browser.desktop.in 
endif

if BOOKMARK
gpe_mini_browser_SOURCES += db-backend.c
else
gpe_mini_browser_SOURCES +=
CFLAGS += -DNOBOOKMARKS
endif

INCLUDES = @MINIBROWSER_CFLAGS@ -DPREFIX=\"@prefix@\" -Wall
LDADD = @MINIBROWSER_LIBS@

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = gpe-mini-browser.png

desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(pixmaps_DATA) \
             hildon/gpe-mini-browser.desktop.in.in \
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
	rm -f gpe-mini-browser.desktop
	rm -f hildon/gpe-mini-browser.desktop
	rm -f po/.intltool-merge-cache 
	
include gpe-dist.am
