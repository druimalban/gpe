PREFIX = /usr/local
PACKAGE = gpe-setbg
DEBUG = yes
CVSBUILD = yes
VERSION = 0.02-1

MEMBERS = $(PACKAGE)

CFLAGS = `pkg-config --cflags gtk+-2.0`

CFLAGS += -D_GNU_SOURCE -Wall
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libgpewidget
LDFLAGS += -L../libgpewidget
BUILD = ../build
else
CFLAGS += -I/usr/include/gpe
BUILD = build
endif
LDFLAGS += -lgpewidget

LDFLAGS += `pkg-config --libs gtk+-2.0`

CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

.PHONY = upstream dpkg ipkg

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install-program: all
	install -Ds $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -D -m 644 gpe-default-bg.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-default-bg.png
	mkdir -p $(DESTDIR)/etc/gpe

clean:
	rm -f $(PACKAGE) $(OBJS)


include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
