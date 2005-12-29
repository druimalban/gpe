SUBDIRS = po

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir)/gpe -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE \
           -DPACKAGE_LOCALE_DIR=\"@gpewhat_localedir@\"

bin_PROGRAMS = gpe-what

gpe_what_LDADD = @DEPS_LIBS@

gpe_what_SOURCES = gpe-what.c

desktopdir = $(datadir)/applications
desktop_in_files = gpe-what.desktop.in

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = gpe-what.png gpe-what-active.png

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
	rm -f gpe-what.desktop
	rm -f po/.intltool-merge-cache 
	rm -rf autom4te.cache
	rm -f *.d *~
	
include gpe-dist.am
include gpe-tp.am
