PREFIX = /usr/local
PACKAGE = gpe-contacts
GTK2 = yes
pkgdatadir = $(PREFIX)/share/$(PACKAGE)
CVSBUILD = yes
STRIP = strip
VERSION = 0.18
DEBUG = no

LINGUAS = de sv cs nl hu ro ru sr sk

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

CPPFLAGS += -D_GNU_SOURCE -DENABLE_NLS

CPPFLAGS += -I../libdisplaymigration
LDFLAGS += -L../libdisplaymigration -ldisplaymigration -L../libgpepimc -lgpepimc -L../libgpevtype -lgpevtype `pkg-config --libs mimedir-1.0`

LDFLAGS += -lsqlite

CFLAGS = $(GTKCFLAGS) -I../libgpepimc -I../libgpewidget -Wall `pkg-config --cflags libxml-2.0` -I../libgpevtype `pkg-config --cflags mimedir-1.0`
CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

MEMBERS = db editing main structure support configure old-db export

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
	install  gpe-contacts $(DESTDIR)$(PREFIX)/bin/gpe-contacts
	$(STRIP) $(DESTDIR)$(PREFIX)/bin/gpe-contacts
	install -d $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 gpe-contacts.desktop $(DESTDIR)$(PREFIX)/share/applications/
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 icons/gpe-contacts.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-contacts.png
	install -d $(DESTDIR)$(pkgdatadir)
	install -m 644 contacts-layout.xml $(DESTDIR)$(pkgdatadir)/default-layout.xml
	install -m 644 well-known-tags $(DESTDIR)$(pkgdatadir)/well-known-tags
	install -d  $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 icons/frame.png $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -m 644 icons/entry.png $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -m 644 icons/notebook.png $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -m 644 icons/export.png $(DESTDIR)$(PREFIX)/share/pixmaps/

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
-include $(DEPS)
