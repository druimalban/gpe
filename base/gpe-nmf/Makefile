PREFIX = /usr/local
PACKAGE = gpe-nmf
DEBUG = no
CVSBUILD = yes
GST = yes
GST_VERSION = 0.8
VERSION = 0.19

MEMBERS = gpe-nmf playlist_edit

GTKCFLAGS = `pkg-config --cflags gtk+-2.0` -DGTK_DISABLE_DEPRECATED
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

PACKAGE_CFLAGS = $(GTKCFLAGS) -D_GNU_SOURCE -Wall
ifeq ($(DEBUG),yes)
PACKAGE_CFLAGS += -O2 -g
PACKAGE_LDFLAGS = -g #-lefence
else
PACKAGE_CFLAGS += -Os -fomit-frame-pointer
endif
PACKAGE_CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\" -I./backend

PACKAGE_CPPFLAGS += -DGST_NMF `pkg-config --cflags gstreamer-$(GST_VERSION)`

ifeq ($(CVSBUILD),yes)
PACKAGE_CFLAGS += -I../libgpewidget
PACKAGE_LDFLAGS += -L../libgpewidget
BUILD = ../build
else
PACKAGE_CFLAGS += -I/usr/include/gpe
BUILD = build
endif
PACKAGE_LDFLAGS += $(GPELIBS) $(GTKLDFLAGS)

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

.PHONY: backend

all: all-mo backend $(PACKAGE)

backend:
	make -C backend PREFIX=$(PREFIX) GST=$(GST) GST_VERSION=$(GST_VERSION) CVSBUILD=$(CVSBUILD)

$(PACKAGE): $(OBJS) $(LIB)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS) -Lbackend -lgpenmf

install-program: all $(PACKAGE).desktop
	make -C backend PREFIX=$(PREFIX) DESTDIR=$(DESTDIR) install
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications

clean:
	make -C backend clean CVSBUILD=$(CVSBUILD)
	rm -f $(PACKAGE) $(OBJS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
