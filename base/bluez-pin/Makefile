PREFIX = /usr/local
PACKAGE = bluez-pin
CVSBUILD = yes
GTK2 = yes
LINGUAS = eo fr id ga wa sv
VERSION = 0.20

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

CFLAGS = `pkg-config --cflags gtk+-2.0` -DGTK_DISABLE_DEPRECATED `pkg-config --cflags dbus-glib-1` -DDBUS_API_SUBJECT_TO_CHANGE
LDFLAGS += `pkg-config --libs gtk+-2.0` `pkg-config --libs dbus-glib-1` -lbluetooth

CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

MEMBERS = main dbus ui

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install-program: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 bt-logo.png $(DESTDIR)$(PREFIX)/share/pixmaps/bt-logo.png

clean:
	rm -f $(PACKAGE) $(OBJS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
