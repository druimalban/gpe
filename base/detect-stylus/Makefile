STRIP ?= strip
PREFIX ?= /usr/local
PACKAGE = detect-stylus
DEBUG = no
CVSBUILD = yes
VERSION = 0.13

MEMBERS = detect-stylus

PACKAGE_CFLAGS = -D_GNU_SOURCE -Wall
ifeq ($(DEBUG),yes)
PACKAGE_CFLAGS += -O2 -g
PACKAGE_LDFLAGS = -g #-lefence
else
PACKAGE_CFLAGS += -Os -fomit-frame-pointer
endif
PACKAGE_CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
PACKAGE_LDFLAGS += `pkg-config --libs x11`

OBJS = $(patsubst %,%.o,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

#export PKG_CONFIG_PATH = $(BUILD)

all: $(PACKAGE) detect-tsdevice

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(STANDARD_LDFLAGS) $(LDFLAGS) $(PACKAGE_LDFLAGS)

detect-tsdevice: detect-tsdevice.c
	$(CC) -o $@ $^ $(STANDARD_CFLAGS) $(CFLAGS) $(PACKAGE_CFLAGS) $(STANDARD_LDFLAGS) $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin/
	$(STRIP) $(PACKAGE)
	install $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)/etc/X11/Xinit.d
	install 70detect-stylus $(DESTDIR)/etc/X11/Xinit.d

clean:
	rm -f $(PACKAGE) $(OBJS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
