PREFIX = /usr/local
PACKAGE = gpe-nmf
DEBUG = no
CVSBUILD = yes

MEMBERS = gpe-nmf playlist_edit

GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

CFLAGS = $(GTKCFLAGS) -D_GNU_SOURCE -Wall
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\" -I../vorbis-fixp/lib -I./backend

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libgpewidget
LDFLAGS += -L../libgpewidget
else
CFLAGS += -I/usr/include/gpe
endif
LDFLAGS += -lgpewidget1 $(GTKLDFLAGS)

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

.PHONY: backend

all: backend $(PACKAGE)

backend:
	make -C backend PREFIX=$(PREFIX)

$(PACKAGE): $(OBJS) $(LIB)
	$(CC) -o $@ $^ $(LDFLAGS) -Lbackend -lgpenmf

install: all
	make -C backend PREFIX=$(PREFIX) DESTDIR=$(DESTDIR) install
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	mkdir -p $(DESTDIR)$(PREFIX)/lib/menu
	install -m 644 $(PACKAGE).menu $(DESTDIR)$(PREFIX)/lib/menu/$(PACKAGE)

clean:
	make -C backend clean
	rm -f $(PACKAGE) $(OBJS)

ipkg:
	rm -rf familiar/dist familiar/dist-mp3 familiar/dist-ogg
	mkdir -p familiar/dist/CONTROL
	mkdir -p familiar/dist-mp3/CONTROL
	mkdir -p familiar/dist-ogg/CONTROL
	cp familiar/control familiar/dist/CONTROL
	cp familiar/postinst familiar/dist/CONTROL
	cp familiar/control.mp3 familiar/dist-mp3/CONTROL/control
	cp familiar/control.ogg familiar/dist-ogg/CONTROL/control
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr install
	mkdir -p familiar/dist-mp3/usr/lib/gpe-nmf/decoders
	mv familiar/dist/usr/lib/gpe-nmf/decoders/d-mad.so familiar/dist-mp3/usr/lib/gpe-nmf/decoders
	mkdir -p familiar/dist-ogg/usr/lib/gpe-nmf/decoders
	mv familiar/dist/usr/lib/gpe-nmf/decoders/d-vorbis.so familiar/dist-ogg/usr/lib/gpe-nmf/decoders
	chown -R root.root familiar/dist
	ipkg-build familiar/dist
	chown -R root.root familiar/dist-mp3
	ipkg-build familiar/dist-mp3
	chown -R root.root familiar/dist-ogg
	ipkg-build familiar/dist-ogg

include ../build/Makefile.translation
