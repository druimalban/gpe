PREFIX = /usr/local
PACKAGE = gpe-filemanager
DEBUG = yes
CVSBUILD = yes
VERSION = 0.10
MIME_PIXMAPS = `cd pixmaps/document-icons && ls *.png`
SPINNER_PIXMAPS = `cd pixmaps/spinner && ls *.png`

MEMBERS = main mime-sql mime-programs-sql

CFLAGS = `pkg-config --cflags gtk+-2.0 gnome-vfs-2.0`

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
BUILD = ../build
else
CFLAGS += -I/usr/include/gpe
BUILD = build
endif
LDFLAGS += -lgpewidget -lsqlite -lgnomevfs-2

LDFLAGS += `pkg-config --libs gtk+-2.0`

CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: gpe-filemanager

gpe-filemanager: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install-program: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -s gpe-filemanager $(DESTDIR)$(PREFIX)/bin/gpe-filemanager
	install -D -m 644 gpe-filemanager.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-filemanager.png
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/
	for i in $(MIME_PIXMAPS); do install -D -m 644 pixmaps/document-icons/$$i $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/filemanager/document-icons/$$i; done
	for i in $(SPINNER_PIXMAPS); do install -D -m 644 pixmaps/spinner/$$i $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/filemanager/spinner/$$i; done
	install -m 644 pixmaps/zoom_in.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/filemanager/zoom_in.png
	install -m 644 pixmaps/zoom_out.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/filemanager/zoom_out.png

uninstall:
	rm -rf $(DESTDIR)$(PREFIX)/bin/gpe-filemanager $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-filemanager.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/filemanager

clean:
	rm -f gpe-filemanager $(OBJS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
