PREFIX = /usr/local
PACKAGE = minitheme
DEBUG = no
CVSBUILD = yes
GTK2 = yes

MEMBERS = minitheme

CFLAGS = `pkg-config --cflags gtk+-2.0`

CFLAGS += -D_GNU_SOURCE -Wall

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libgpewidget -I../libxsettings-client -I../libxsettings
LDFLAGS += -L../libgpewidget -L../libxsettings-client -L../libxsettings
BUILD = ../build
else
BUILD = build
endif
LDFLAGS += -lgpewidget `pkg-config --libs gtk+-2.0` -lXsettings-client

CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: $(PACKAGE) all-mo

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install-program: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/

clean:
	rm -f $(PACKAGE) $(OBJS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
