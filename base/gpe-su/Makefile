PREFIX = /usr/local
PACKAGE = gpe-su
DEBUG = yes
CVSBUILD = yes
VERSION = 0.08

MEMBERS = gpe-su

GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0` -lgpewidget -lutil

CPPFLAGS += -D_GNU_SOURCE

CFLAGS = $(GTKCFLAGS) -g -I../libgpewidget -Wall -I../libdisplaymigration
CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = $(GTKLDFLAGS) -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
LDFLAGS = $(GTKLDFLAGS)
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libgpewidget
LDFLAGS += -L../libgpewidget
BUILD = ../build
else
CFLAGS += -I/usr/include/gpe
BUILD = build
endif

OBJS = $(patsubst %,%.o,$(MEMBERS))

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

install-program: all $(PACKAGE).desktop
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
