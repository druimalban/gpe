VERSION = 1.0.4

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

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

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

install-program: all $(PACKAGE)-play.desktop  $(PACKAGE)-record.desktop
	install -Ds $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -D $(PACKAGE)-play $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)-play
	install -D $(PACKAGE)-record $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)-record
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE)-play.desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE)-play.desktop
	install -m 644 $(PACKAGE)-record.desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE)-record.desktop
	install -D -m 644 gpe-soundbite-play.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-soundbite-play.png
	install -D -m 644 gpe-soundbite-record.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-soundbite-record.png

clean:
	rm -f $(PACKAGE) $(OBJS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
