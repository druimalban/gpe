PREFIX = /usr/local
PACKAGE = gpe-contacts
GTK2 = yes
pkgdatadir = $(PREFIX)/share/$(PACKAGE)
CVSBUILD = yes
VERSION = 0.12

GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

ifeq ($(DEBUG),yes)
LDFLAGS += -g
CFLAGS += -O2 -g
else
CFLAGS += -Os -fomit-frame-pointer
endif

CPPFLAGS += -D_GNU_SOURCE

CPPFLAGS += -I../libdm
LDFLAGS += -L../libdm -ldm

LDFLAGS += -lsqlite

CFLAGS = $(GTKCFLAGS) -I../libgpewidget -Wall `pkg-config --cflags libxml-2.0`
CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

MEMBERS = callbacks db editing interface main structure support

DEPS = $(patsubst %,%.d,$(MEMBERS))
OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: gpe-contacts all-mo

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ -L../libgpewidget -lgpewidget $(GTKLDFLAGS) `pkg-config --libs libxml-2.0` $(LDFLAGS)

clean:
	rm -f gpe-contacts $(OBJS) $(DEPS)

install-program: all $(PACKAGE).desktop
	install -d $(DESTDIR)$(PREFIX)/bin
	install -s gpe-contacts $(DESTDIR)$(PREFIX)/bin/gpe-contacts
	install -d $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 gpe-contacts.desktop $(DESTDIR)$(PREFIX)/share/applications/gpe-contacts
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 icons/gpe-contacts.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-contacts.png
	install -d $(DESTDIR)$(pkgdatadir)
	install -m 644 contacts-layout.xml $(DESTDIR)$(pkgdatadir)/default-layout.xml
	install -m 644 well-known-tags $(DESTDIR)$(pkgdatadir)/well-known-tags
	install -d  $(DESTDIR)$(pkgdatadir)/pixmaps
	install -m 644 icons/frame.png $(DESTDIR)$(pkgdatadir)/pixmaps/
	install -m 644 icons/entry.png $(DESTDIR)$(pkgdatadir)/pixmaps/
	install -m 644 icons/notebook.png $(DESTDIR)$(pkgdatadir)/pixmaps/

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
-include $(DEPS)
