PREFIX = /usr/local
PACKAGE = bluez-pin
CVSBUILD = yes
GTK2 = yes
LINGUAS = eo fr id
VERSION = 0.10

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

GTKCFLAGS = `pkg-config --cflags gtk+-2.0` -DGTK_DISABLE_DEPRECATED
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

SOURCES = $(PACKAGE).c

all: $(PACKAGE)

$(PACKAGE): $(PACKAGE).o
	$(CC) -o $@ $< $(GTKLDFLAGS) -lsqlite

$(PACKAGE).o: $(PACKAGE).c
	$(CC) -c -o $@ $< $(CPPFLAGS) $(GTKCFLAGS) -Os -Wall -fomit-frame-pointer

install-program: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 664 bt-logo.png $(DESTDIR)$(PREFIX)/share/pixmaps/bt-logo.png

clean:
	rm -f $(PACKAGE) $(PACKAGE).o

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
