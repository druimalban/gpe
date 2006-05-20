PREFIX = /usr
PACKAGE = gpe-plucker
DEBUG = no
CVSBUILD = yes
GST = yes

VERSION = 0.4

MEMBERS = gpe-plucker

GTKCFLAGS = `pkg-config --cflags x11 gtk+-2.0` -DGTK_DISABLE_DEPRECATED

GTKLDFLAGS += `pkg-config --libs x11 gtk+-2.0` -lX11 -lXrender -lXext

PACKAGE_CFLAGS = $(GTKCFLAGS) -Iunpluck/ -DUSE_GPE -D_GNU_SOURCE -Wall
ifeq ($(DEBUG),yes)
PACKAGE_CFLAGS += -O2 -g
PACKAGE_LDFLAGS = -g
else
PACKAGE_CFLAGS += -Os -fomit-frame-pointer
endif
PACKAGE_CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

ifeq ($(CVSBUILD),yes)
PACKAGE_CFLAGS += -I../libgpewidget
PACKAGE_LDFLAGS += -L../libgpewidget
BUILD = ../build
else
BUILD = build
endif
PACKAGE_LDFLAGS += -lgpewidget $(GTKLDFLAGS) -Lunpluck/ -lunpluck -lz

SOURCES = image.c library.c viewer.c
OBJS = ${SOURCES:.c=.o}

all: unpluck $(PACKAGE)

unpluck:
	make -C unpluck CVSBUILD=$(CVSBUILD)

$(PACKAGE): $(OBJS) $(LIB)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: all $(PACKAGE).desktop
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	#mkdir -p $(DESTDIR)$(PREFIX)/share/$(PACKAGE)
	#install -m 644 pixmaps/*.png $(DESTDIR)$(PREFIX)/share/$(PACKAGE)/
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications

clean:
	make -C unpluck CVSBUILD=$(CVSBUILD) clean 
	rm -f $(PACKAGE) $(OBJS) $(PACKAGE).desktop *.ipk
	rm -rf familiar/dist

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg

.PHONY: unpluck
