SUBDIRS = po

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir)/gpe -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE \
           -DPACKAGE_LOCALE_DIR=\"@gpesynctool_localedir@\"

bin_PROGRAMS = gpe-synctool

gpe_synctool_LDADD = @DEPS_LIBS@

gpe_synctool_SOURCES = main.c

desktopdir = $(datadir)/applications
desktop_in_files = gpe-synctool.desktop.in

pixmapsdir = $(datadir)/pixmaps/gpe/default
pixmaps_DATA = pixmaps/sync-on.png \
               pixmaps/sync-off.png

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@


EXTRA_DIST = $(desktop_DATA) \
             $(pixmaps_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in \
             po/gpe-synctool.pot
			 
DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	rm -f gpe-synctool.desktop
	rm -rf autom4te.cache
	rm -f *.d *~
	
include gpe-dist.am
include gpe-tp.am
