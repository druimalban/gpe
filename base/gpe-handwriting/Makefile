PREFIX = /usr/local
PACKAGE = gpe-handwriting
DEBUG = no
CVSBUILD = yes
GTK2 = yes

MEMBERS = main

ifeq ($(GTK2),yes)
GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`
else
GTKCFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags`
GTKLDFLAGS = `gtk-config --libs`
endif

CFLAGS = $(GTKCFLAGS) -D_GNU_SOURCE -Wall
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../../base/libgpewidget
LDFLAGS += -L../../base/libgpewidget
BUILD = ../build
else
CFLAGS += -I/usr/include/gpe
BUILD = build
endif
LDFLAGS += -lgpewidget $(GTKLDFLAGS)

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install-program: all
	install -Ds $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -D $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	install -d $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$(PACKAGE)
	install -m 644 pixmaps/zoom_in.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$(PACKAGE)/zoom_in.png
	install -m 644 pixmaps/zoom_out.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$(PACKAGE)/zoom_out.png
	install -D $(PACKAGE).menu $(DESTDIR)$(PREFIX)/lib/menu/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE)-play

clean:
	rm -f $(PACKAGE) $(OBJS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
