PACKAGE = gpe-othello
PREFIX ?= /usr/local
DEBUG = no
LINGUAS = 
VERSION = 0.2-1

MEMBERS = callbacks gameutils moteur deepblue support globals main

GTKCFLAGS = `pkg-config --cflags gtk+-2.0` -DGTK_ENABLE_BROKEN
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

PACKAGE_CPPFLAGS = $(GTKCFLAGS) -D_GNU_SOURCE 
ifeq ($(DEBUG),yes)
PACKAGE_CFLAGS += -O2 -g
PACKAGE_LDFLAGS = -g
else
PACKAGE_CFLAGS += -Os -fomit-frame-pointer
endif
PACKAGE_CFLAGS += -Wall -I../../base/libgpewidget
PACKAGE_CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
PACKAGE_CFLAGS += -MD

PACKAGE_LDFLAGS += -L../../base/libgpewidget -lgpewidget

.SUFFIXES: .d

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

PIXMAPS = bell.png

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^  $(GTKLDFLAGS) $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: all 
	install -D $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -d $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/
	install -d $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$(PACKAGE)
	install -m 644 pixmaps/*.xpm $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$(PACKAGE)
	
clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include ../../base/build/Makefile.dpkg_ipkg
-include $(DEPS)
