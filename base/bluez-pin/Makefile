GPE = yes
PREFIX = /usr/local
PACKAGE = bluez-pin
GTK2 = yes

ifeq ($(GTK2),yes)
GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`
else
GTKCFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags`
GTKLDFLAGS = `gtk-config --libs`
endif

CPPFLAGS = -I../libgpewidget
ifeq ($(GPE),yes)
CPPFLAGS += -DUSE_LIBGPEWIDGET
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

SOURCES = bluez-pin.c

all: bluez-pin

bluez-pin: bluez-pin.o
	$(CC) -o $@ $< -L../libgpewidget -lgpewidget $(GTKLDFLAGS) -lsqlite

bluez-pin.o: bluez-pin.c
	$(CC) -c -o $@ $< $(CPPFLAGS) $(GTKCFLAGS) -Os -Wall -fomit-frame-pointer

ifneq ($(GPE),yes)
install:
	install -D bt-logo.png $(DESTDIR)/usr/share/pixmaps/bt-logo.png
endif

install: all install-mo
	install -D bluez-pin $(DESTDIR)/usr/bin/bluez-pin
	strip $(DESTDIR)/usr/bin/bluez-pin

clean:
	rm -f bluez-pin bluez-pin.o

include ../build/Makefile.translation
include ../build/Makefile.dpkg_ipkg
