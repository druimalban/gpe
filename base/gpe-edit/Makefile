PREFIX = /usr/local
PACKAGE = gpe-edit
DEBUG = no
CVSBUILD = yes
VERSION = 0.18

MEMBERS = main
LINGUAS = id pt de zh_TW fr ro wa sv

CFLAGS = `pkg-config --cflags gtk+-2.0`
CONTROL = control1

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
else
CFLAGS += -I/usr/include/gpe
endif
LDFLAGS += -lgpewidget

LDFLAGS += `pkg-config --libs gtk+-2.0`

CFLAGS += -I../libdm
LDFLAGS += -L../libdm -ldm

CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: gpe-edit all-mo

gpe-edit: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install-program: all gpe-edit.desktop
	install -d $(DESTDIR)$(PREFIX)/bin
	install -s gpe-edit $(DESTDIR)$(PREFIX)/bin/gpe-edit
	install -D -m 644 gpe-edit.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-edit.png
	install -D -m 644 gpe-edit.mime $(DESTDIR)/etc/mime-handlers/gpe-edit.mime
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/

clean: clean-po
	rm -f gpe-edit $(OBJS) $(PACKAGE).desktop

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
