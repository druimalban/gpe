PREFIX = /usr/local
PACKAGE = gpe-soundbite
DEBUG = yes
CVSBUILD = yes
ESD = yes

MEMBERS = main gsm-codec
ifeq ($(ESD),yes)
MEMBERS += esd
else
MEMBERS += oss
endif

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
CFLAGS += -I../libgpewidget
LDFLAGS += -L../libgpewidget -lgsm
else
CFLAGS += -I/usr/include/gpe
endif
LDFLAGS += -lgpewidget -lgsm $(GTKLDFLAGS)

ifeq ($(ESD),yes)
LDFLAGS += -lesd
endif

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install: all install-mo
	install -D $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -D $(PACKAGE).menu $(DESTDIR)$(PREFIX)/lib/menu/$(PACKAGE)
	install -D $(PACKAGE)-record.menu $(DESTDIR)$(PREFIX)/lib/menu/$(PACKAGE)-record
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE)-play.desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE)-play.desktop
	install -m 644 $(PACKAGE)-record.desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE)-record.desktop
	install -D gpe-soundbite-play.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-soundbite-play.png
	install -D gpe-soundbite-record.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-soundbite-record.png
	strip $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)

clean:
	rm -f $(PACKAGE) $(OBJS)

include ../build/Makefile.dpkg_ipkg
include ../build/Makefile.translation
