PREFIX = /usr/local
PACKAGE = gpe-filemanager
DEBUG = yes
CVSBUILD = yes
MIME_PIXMAPS = `cd pixmaps/document-icons && ls *.png`
SPINNER_PIXMAPS = `cd pixmaps/spinner && ls *.png`
GTK2 = yes

MEMBERS = main mime-sql

ifeq ($(GTK2),yes)
CFLAGS = `pkg-config --cflags gtk+-2.0` -DGTK_ENABLE_BROKEN
CONTROL = control1
else
CFLAGS = `gdk-pixbuf-config --cflags` `gtk-config --cflags` 
CONTROL = control
endif

CFLAGS += -D_GNU_SOURCE -Wall
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libgpewidget
LDFLAGS += -L../libgpewidget
else
CFLAGS += -I/usr/include/gpe
endif
LDFLAGS += -lgpewidget -lsqlite

ifeq ($(GTK2),yes)
LDFLAGS += `pkg-config --libs gtk+-2.0`
else
LDFLAGS += `gtk-config --libs` `gdk-pixbuf-config --libs`
endif

CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: gpe-filemanager

gpe-filemanager: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install: all
	install -D gpe-filemanager $(DESTDIR)$(PREFIX)/bin/gpe-filemanager
	install -D -m 644 gpe-filemanager.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-filemanager.png
	install -D -m 644 gpe-filemanager.menu $(DESTDIR)$(PREFIX)/lib/menu/gpe-filemanager
	for i in $(MIME_PIXMAPS); do install -D pixmaps/document-icons/$$i $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$(PACKAGE)/document-icons/$$i; done
	for i in $(SPINNER_PIXMAPS); do install -D pixmaps/spinner/$$i $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$(PACKAGE)/spinner/$$i; done
	strip $(DESTDIR)$(PREFIX)/bin/gpe-filemanager

uninstall:
	rm -rf $(DESTDIR)$(PREFIX)/bin/gpe-filemanager $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-filemanager.png $(DESTDIR)$(PREFIX)/lib/menu/gpe-filemanager $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$(PACKAGE) ~/.gpe/mime_types

clean:
	rm -f gpe-filemanager $(OBJS)

include ../build/Makefile.dpkg_ipkg
include ../build/Makefile.translation
