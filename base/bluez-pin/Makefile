PREFIX = /usr/local
SYSCONFDIR=$(PREFIX)/etc
PACKAGE = bluez-pin
CVSBUILD = yes
GTK2 = yes
LINGUAS = zh_CN ga hu fi fr el eo cs de vi tr uk sk sv sr af rm ro ru rw wa pl nl pt_BR id
VERSION = 0.25

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

CFLAGS += -O2 -g

CFLAGS += `pkg-config --cflags gtk+-2.0 libglade-2.0 dbus-glib-1 gconf-2.0` -DGTK_DISABLE_DEPRECATED -DDBUS_API_SUBJECT_TO_CHANGE
LDFLAGS += `pkg-config --libs gtk+-2.0 libglade-2.0 dbus-glib-1 gconf-2.0` -lbluetooth
CFLAGS += -Wall

CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

MEMBERS = main pin-dbus pin-ui
GLADE = bluez-pin.glade bluez-pin-gpe.glade

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS)) bluez.conf $(GLADE)

all: $(PACKAGE) bluez.conf

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install-program: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(SYSCONFDIR)/dbus-1/system.d/
	install -m 644 bluez.conf $(DESTDIR)$(SYSCONFDIR)/dbus-1/system.d/
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 bt-logo.png $(DESTDIR)$(PREFIX)/share/pixmaps/bt-logo.png
	mkdir -p $(DESTDIR)$(PREFIX)/share/bluez-pin/
	install -m 644 $(GLADE) $(DESTDIR)$(PREFIX)/share/bluez-pin/

clean:
	rm -f $(PACKAGE) $(OBJS) *.bak *.gladep

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
