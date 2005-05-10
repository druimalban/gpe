
SUBDIRS = po

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir)/gpe -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE \
           -DPACKAGE_LOCALE_DIR=\"@gpetodo_localedir@\"

bin_PROGRAMS = gpe-todo

gpe_todo_LDADD = @DEPS_LIBS@

gpe_todo_SOURCES = list-ui.c main.c todo-ui.c todo.h

desktopdir = $(datadir)/applications
desktop_in_files = gpe-todo.desktop.in

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/gpe-todo

pixmaps_DATA = pixmaps/bar-box.png \
               pixmaps/dot-box.png \
               pixmaps/notick-box.png \
               pixmaps/tick-box.png \
               pixmaps/flag-16.png

pixmaps_extradir = $(datadir)/pixmaps
pixmaps_extra_DATA = pixmaps/gpe-todo.png


EXTRA_DIST = $(desktop_DATA) \
             $(pixmaps_DATA) \
             $(pixmaps_extra_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in
			 

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	rm -f gpe-todo.desktop
	rm -f po/.intltool-merge-cache 
	rm -rf autom4te.cache
	rm -f *.d *~

include gpe-dist.am
include gpe-tp.am
