PREFIX = /usr/local
PACKAGE = bluez-pin
GTK2 = yes
VERSION = 0.08

GTKCFLAGS = `pkg-config --cflags gtk+-2.0` -DGTK_DISABLE_DEPRECATED
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

SOURCES = bluez-pin.c

all: bluez-pin

bluez-pin: bluez-pin.o
	$(CC) -o $@ $< $(GTKLDFLAGS) -lsqlite

bluez-pin.o: bluez-pin.c
	$(CC) -c -o $@ $< $(CPPFLAGS) $(GTKCFLAGS) -Os -Wall -fomit-frame-pointer

install: all install-mo
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s bluez-pin $(DESTDIR)$(PREFIX)/bin/bluez-pin
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 664 bt-logo.png $(DESTDIR)$(PREFIX)/share/pixmaps/bt-logo.png

clean:
	rm -f bluez-pin bluez-pin.o

include ../build/Makefile.translation
include ../build/Makefile.dpkg_ipkg
