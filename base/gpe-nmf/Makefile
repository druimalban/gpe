PREFIX = /usr/local
PACKAGE = gpe-nmf
DEBUG = no
CVSBUILD = yes
GST = yes
VERSION = 0.16

MEMBERS = gpe-nmf playlist_edit

GTKCFLAGS = `pkg-config --cflags gtk+-2.0` -DGTK_DISABLE_DEPRECATED
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

CFLAGS = $(GTKCFLAGS) -D_GNU_SOURCE -Wall
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\" -I./backend

CPPFLAGS += -DGST_NMF `pkg-config --cflags gstreamer-0.6`

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libgpewidget
LDFLAGS += -L../libgpewidget
BUILD = ../build
else
CFLAGS += -I/usr/include/gpe
BUILD = build
endif
LDFLAGS += -lgpewidget $(GTKLDFLAGS)

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

.PHONY: backend

all: all-mo backend $(PACKAGE)

backend:
	make -C backend PREFIX=$(PREFIX) GST=$(GST)

$(PACKAGE): $(OBJS) $(LIB)
	$(CC) -o $@ $^ $(LDFLAGS) -Lbackend -lgpenmf

install-program: all $(PACKAGE).desktop
	make -C backend PREFIX=$(PREFIX) DESTDIR=$(DESTDIR) install
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications

clean:
	make -C backend clean
	rm -f $(PACKAGE) $(OBJS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
