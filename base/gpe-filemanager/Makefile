PREFIX = /usr/local
PACKAGE = gpe-filemanager
DEBUG = no
CVSBUILD = yes
VERSION = 0.26
MIME_PIXMAPS = `cd pixmaps/document-icons && ls *.png`

LINGUAS = es sl sv vi ko ja

MEMBERS = main bluetooth guitools fileops

ifeq ($(CVSBUILD),yes)
PACKAGE_CFLAGS += -I ../libgpewidget
PACKAGE_LDFLAGS += -L ../libgpewidget
BUILD = ../build
else
BUILD = build
endif

ifneq (,$(shell pkg-config hildon-libs --modversion 2>/dev/null))
HILDON_CFLAGS += `pkg-config hildon-libs libossomime libosso --cflags`
HILDON_CPPFLAGS += -DUSE_HILDON
HILDON_LDFLAGS += `pkg-config hildon-libs libossomime libosso --libs`
HILDON_INSTALL_SUFFIX = "hildon/"
endif
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS) `pkg-config --cflags dbus-glib-1 gnome-vfs-2.0 gtk+-2.0 gnome-vfs-module-2.0 libgpewidget` $(HILDON_CFLAGS)
PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS) -DDBUS_API_SUBJECT_TO_CHANGE -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\" -DVERSION=\"$(VERSION)\" $(HILDON_CPPFLAGS)
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS) `pkg-config --libs dbus-glib-1 gtk+-2.0 gnome-vfs-2.0 gthread-2.0 libgpewidget` -Wl --export-dynamic $(HILDON_LDFLAGS)

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))

all: $(PACKAGE) $(PACKAGE).desktop

gpe-filemanager: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -s gpe-filemanager $(DESTDIR)$(PREFIX)/bin/gpe-filemanager
	install -D -m 644 gpe-filemanager.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-filemanager.png
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications/$(HILDON_INSTALL_SUFFIX)
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/$(HILDON_INSTALL_SUFFIX)
	mkdir -p $(DESTDIR)$(PREFIX)/share/dbus-1/services
	install -m 644 $(PACKAGE).service $(DESTDIR)$(PREFIX)/share/dbus-1/services/
	for i in $(MIME_PIXMAPS); do install -D -m 644 pixmaps/document-icons/$$i $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/filemanager/document-icons/$$i; done

uninstall:
	rm -rf $(DESTDIR)$(PREFIX)/bin/gpe-filemanager $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-filemanager.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/filemanager

clean:
	rm -f gpe-filemanager $(OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
