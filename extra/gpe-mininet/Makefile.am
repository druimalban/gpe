SUBDIRS = po

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir)/gpe -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE \
           -DPACKAGE_LOCALE_DIR=\"@gpemininet_localedir@\"

bin_PROGRAMS = gpe-mininet

gpe_mininet_LDADD = @DEPS_LIBS@

gpe_mininet_SOURCES = main.c netlink.c iwlib.c main.h iwlib.h netlink.h

desktopdir = $(datadir)/applications
desktop_in_files = gpe-mininet.desktop.in

pixmapsdir = $(datadir)/gpe/pixmaps/default
pixmaps_DATA = pixmaps/gpe-mininet.png \
               pixmaps/net-off.png \
               pixmaps/net-on.png \
               pixmaps/net-off-48.png \
               pixmaps/net-on-48.png

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@


EXTRA_DIST = $(desktop_DATA) \
             $(pixmaps_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in
			 
DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	rm -f gpe-mininet.desktop
	rm -f po/.intltool-merge-cache 
	rm -rf autom4te.cache
	rm -f *.d *~
	
include gpe-dist.am
include gpe-tp.am
