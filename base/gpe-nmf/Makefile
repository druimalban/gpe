PREFIX = /usr/local
PACKAGE = gpe-nmf
#DEBUG = yes
CVSBUILD = yes

MEMBERS = gpe-nmf

DECODER_MEMBERS = decoder stream file_stream
PLAYLIST_MEMBERS = playlist_db playlist_xml playlist_ogg playlist_id3
LIBMEMBERS = player $(patsubst %,decoder/%,$(DECODER_MEMBERS)) $(patsubst %,playlist/%,$(PLAYLIST_MEMBERS))

GTKCFLAGS = `pkg-config --cflags gtk+-2.0` -DGTK_ENABLE_BROKEN
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

CFLAGS = $(GTKCFLAGS) -D_GNU_SOURCE -Wall
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\" -I../vorbis-fixp/lib -I./playlist -I./decoder

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libgpewidget
LDFLAGS += -L../libgpewidget
else
CFLAGS += -I/usr/include/gpe
endif
LDFLAGS += -lgpewidget1 $(GTKLDFLAGS) -lxml -lvorbis -lid3tag -lesd

OBJS = $(patsubst %,%.o,$(MEMBERS))
LIBOBJS = $(patsubst %,%.o,$(LIBMEMBERS))

LIB = libgpe-nmf.so.0

all: $(PACKAGE)

$(PACKAGE): $(OBJS) $(LIB)
	$(CC) -o $@ $^ $(LDFLAGS)

$(LIB): $(LIBOBJS)
	$(CC) -shared -o $@ $^

install: all
	install -D $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	strip $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)

upstream: clean
	( VERSION=`dpkg-parsechangelog | grep ^Version | cut -d\  -f2 | cut -d- -f1`; cd ..; cp -a $(PACKAGE) $(PACKAGE)-$$VERSION.orig; tar czf $(PACKAGE)_$$VERSION.orig.tar.gz --exclude=$(PACKAGE)-$$VERSION.orig/debian --exclude=$(PACKAGE)-$$VERSION.orig/CVS --exclude=$(PACKAGE)-$$VERSION.orig/familiar $(PACKAGE)-$$VERSION.orig ; rm -rf $(PACKAGE)-$$VERSION.orig )

dpkg: upstream
	( VERSION=`dpkg-parsechangelog | grep ^Version | cut -d\  -f2 | cut -d- -f1`; TMP=/var/tmp/debian/$(PACKAGE); PV=$(PACKAGE)-$$VERSION; rm -rf $$TMP; mkdir -p $$TMP; cp ../$(PACKAGE)_$$VERSION.orig.tar.gz $$TMP; (cd $$TMP; tar xzf $(PACKAGE)_$$VERSION.orig.tar.gz ; mv $$PV.orig $$PV ); mkdir -p $$TMP/$$PV/debian; cp debian/* $$TMP/$$PV/debian; cd $$TMP/$$PV; fakeroot dpkg-buildpackage  ; cd ..; rm -rf $$PV)

ipkg:
	rm -rf familiar/dist
	mkdir -p familiar/dist/CONTROL
	cp familiar/control familiar/dist/CONTROL
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr install
	chown -R root.root familiar/dist
	ipkg-build familiar/dist

clean:
	rm -f $(PACKAGE) $(OBJS)
