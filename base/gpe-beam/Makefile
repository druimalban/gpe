PREFIX = /usr/local
PACKAGE = gpe-beam
DEBUG = no
CVSBUILD = yes

all: vcard-export vcard-import vcard-import-file

GTKCFLAGS = `pkg-config --cflags mimedir-1.0`
GTKLDFLAGS += -lmimedir -lglib-2.0

CFLAGS = $(GTKCFLAGS) -D_GNU_SOURCE -Wall
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

LDFLAGS += $(GTKLDFLAGS) -lsqlite

vcard-export: send-vcard.o vcard.o
	$(CC) -o $@ $^ $(LDFLAGS)

vcard-import: import-vcard.o vcard.o
	$(CC) -o $@ $^ $(LDFLAGS)

vcard-import-file: import-vcard-file.o vcard.o
	$(CC) -o $@ $^ $(LDFLAGS)
