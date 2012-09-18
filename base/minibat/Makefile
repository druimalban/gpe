PACKAGE = minibat
PREFIX = /usr/local
DEBUG = yes
CVSBUILD = yes
VERSION = 0.18

PKGS = x11 cairo libmb xext xrender

LINGUAS = 

MEMBERS = minibat mbtray findargb

CPPFLAGS = -D_GNU_SOURCE -DHAVE_APM_H `pkg-config --cflags $(PKGS)`

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS += -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -Wall


CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

.SUFFIXES: .d

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

export PKG_CONFIG_PATH = $(BUILD)

all: $(PACKAGE) all-mo

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ `pkg-config --libs $(PKGS)` $(LDFLAGS)

install-program: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
#install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE).desktop
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -m 644 $(PACKAGE)-ac.png $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -m 644 $(PACKAGE)-battery.png $(DESTDIR)$(PREFIX)/share/pixmaps/

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
