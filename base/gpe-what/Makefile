PREFIX = /usr/local
PACKAGE = gpe-what
#DEBUG = yes
CVSBUILD = yes
VERSION = 0.26

MEMBERS = gpe-what

GTKCFLAGS = `pkg-config --cflags glib-2.0` `pkg-config --cflags pangoxft` `pkg-config --cflags xft`
GTKLDFLAGS += `pkg-config --libs glib-2.0` `pkg-config --libs pangoxft` `pkg-config --libs xft`
CONTROL = control

CFLAGS = $(GTKCFLAGS) -D_GNU_SOURCE -Wall
LDFLAGS = $(GTKLDFLAGS)

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS += -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

LDFLAGS += -lmb

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install-program: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps

clean:
	rm -f $(PACKAGE) $(OBJS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
