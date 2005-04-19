
SUBDIRS = doc po

lib_LTLIBRARIES = libgpewidget.la

INCLUDES = @GPEWIDGET_CFLAGS@ -I$(top_srcdir)/gpe -I$(top_srcdir) \
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE \
           -DPACKAGE_LOCALE_DIR=\"@datadir@/locale/@PACKAGE_NAME@\" -Wall

libgpewidget_la_SOURCES = gtkdatecombo.c errorbox.c init.c pixmaps.c smallbox.c \
                          dirbrowser.c picturebutton.c stylus.c screensize.c \
						  question.c gtksimplemenu.c spacing.c tray.c \
						  translabel.c popup.c popup_menu.c gpetimesel.c \
						  gpeiconlistitem.c gpeclockface.c gpe-iconlist.c \
						  gpewindowlist.c gpehelp.c gpeiconlistview.c \
						  gpewindowbits.c infoprint.c link-warning.h
						  
libgpewidget_la_LDFLAGS = @GPEWIDGET_LIBS@ -version-info 1:0:0

gpeincludedir = $(includedir)/gpe
gpeinclude_HEADERS = gpe/init.h gpe/errorbox.h gpe/smallbox.h gpe/pixmaps.h gpe/gtkdatecombo.h \
                     gpe/dirbrowser.h gpe/stylus.h gpe/picturebutton.h gpe/spacing.h \
					 gpe/translabel.h gpe/question.h gpe/windows.h gpe/gpe-iconlist.h \
					 gpe/gtksimplemenu.h gpe/gpewindowlist.h gpe/tray.h gpe/popup.h \
					 gpe/gpeiconlistview.h gpe/gpeclockface.h gpe/gpeiconlistitem.h \
					 gpe/infoprint.h gpe/gpetimesel.h


pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgpewidget.pc

pixmapdir = $(datadir)/pixmaps

pixmap_DATA = pixmaps/clock.png pixmaps/clock24.png pixmaps/day-night-wheel.png

EXTRA_DIST = $(pixmap_DATA) \
             intltool-extract.in \
             intltool-merge.in \
             intltool-update.in\
			 libtool-cross.patch

clean-local:
	rm -rf po/.intltool-merge-cache *.d autom4te.cache

DISTCLEANFILES = intltool-extract intltool-merge intltool-update


doc-local:
	make -C doc
	
include gpe-dist.am
include gpe-tp.am

.PHONY: doc
