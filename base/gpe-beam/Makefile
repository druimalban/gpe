PREFIX = /usr/local
PACKAGE = gpe-beam
DEBUG = no
CVSBUILD = yes

all: send-vcard import-vcard

GTKCFLAGS = `pkg-config --cflags mimedir-1.0`
GTKLDFLAGS += `pkg-config --libs mimedir-1.0`

CFLAGS = $(GTKCFLAGS) -D_GNU_SOURCE -Wall
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

LDFLAGS += $(GTKLDFLAGS) -lsqlite

send-vcard: send-vcard.o vcard.o
	$(CC) -o $@ $^ $(LDFLAGS) -lopenobex

import-vcard: import-vcard.o vcard.o
	$(CC) -o $@ $^ $(LDFLAGS)

