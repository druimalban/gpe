
SUBDIRS = po

ENABLE_HILDON = @ENABLE_HILDON@

if HILDON
DEPS_CFLAGS += -DIS_HILDON
SUBDIRS += hildon
endif

INCLUDES = $(DEPS_CFLAGS) $(HILDON_CFLAGS) -I$(top_srcdir)/gpe -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE \
           -DPACKAGE_LOCALE_DIR=\"@gpetodo_localedir@\"

bin_PROGRAMS = gpe-todo

gpe_todo_LDADD = @DEPS_LIBS@ @HILDON_LIBS@

gpe_todo_SOURCES = list-ui.c main.c todo-ui.c todo.h

if HILDON

desktopdir = 
desktop_in_files = 

# we need a 26x26 icon here
pixmaps_extra_DATA = 

else

desktopdir = $(datadir)/applications
desktop_in_files = gpe-todo.desktop.in

pixmaps_extra_DATA = pixmaps/gpe-todo.png

endif

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/gpe-todo

pixmaps_DATA = pixmaps/bar-box.png \
               pixmaps/dot-box.png \
               pixmaps/notick-box.png \
               pixmaps/tick-box.png \
               pixmaps/flag-16.png

pixmaps_extradir = $(datadir)/pixmaps


EXTRA_DIST = gpe-todo.desktop.in \
             $(pixmaps_DATA) \
             hildon/gpe-todo.desktop.in \
             $(pixmaps_extra_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in \
             hildon/gpe-todo.desktop.in.in \
             hildon/com.nokia.gpe-todo.service.in \
             hildon/gpe-todo.png \
	     po/gpe-todo.pot

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update

clean-local:
	rm -f gpe-todo.desktop hildon/gpe-todo.desktop
	rm -f po/.intltool-merge-cache 
	rm -rf autom4te.cache
	rm -f *.d *~

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar xzf $(top_builddir)/../$(PACKAGE)-$(VERSION).tar.gz
	cp -r $(top_builddir)/debian $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)

include gpe-dist.am
include gpe-tp.am
