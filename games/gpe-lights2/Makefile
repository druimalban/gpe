PACKAGE = gpe-lights
PREFIX = /usr/local
DEBUG = no
LINGUAS = 

MEMBERS = gpe-lights

GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`
CONTROL = control

CPPFLAGS = $(GTKCFLAGS) -D_GNU_SOURCE 
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -Wall
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

LDFLAGS += -L../../base/libgpewidget -lgpewidget

.SUFFIXES: .d

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^  $(GTKLDFLAGS) $(LDFLAGS)

install: all 
	install -D $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	strip $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	install -d $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications
	install -d $(DESTDIR)$(PREFIX)/share/$(PACKAGE)
	install -m 644 bulbs.png $(DESTDIR)$(PREFIX)/share/$(PACKAGE)
	install -m 644 oses.png $(DESTDIR)$(PREFIX)/share/$(PACKAGE)
	install -m 644 colors.png $(DESTDIR)$(PREFIX)/share/$(PACKAGE)
	install -m 644 face.png $(DESTDIR)$(PREFIX)/share/$(PACKAGE)

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include ../../base/build/Makefile.dpkg_ipkg
-include $(DEPS)
