PREFIX = /usr/local
PACKAGE = gpe-filemanager
DEBUG = no
CVSBUILD = yes
VERSION = 0.21
MIME_PIXMAPS = `cd pixmaps/document-icons && ls *.png`

MEMBERS = main bluetooth guitools

ifeq ($(CVSBUILD),yes)
PACKAGE_CFLAGS += -I ../libgpewidget
PACKAGE_LDFLAGS += -L ../libgpewidget
BUILD = ../build
else
BUILD = build
endif

PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS) `pkg-config --cflags dbus-glib-1 gnome-vfs-2.0 gtk+-2.0 gnome-vfs-module-2.0` 
PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS) -DDBUS_API_SUBJECT_TO_CHANGE -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\" -DVERSION=\"$(VERSION)\"
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS) `pkg-config --libs dbus-glib-1 gtk+-2.0` -pthread -Wl --export-dynamic -lgnomevfs-2 -lgobject-2.0 -lgthread-2.0  

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))

all: gpe-filemanager

gpe-filemanager: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -s gpe-filemanager $(DESTDIR)$(PREFIX)/bin/gpe-filemanager
	install -D -m 644 gpe-filemanager.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-filemanager.png
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/
	for i in $(MIME_PIXMAPS); do install -D -m 644 pixmaps/document-icons/$$i $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/filemanager/document-icons/$$i; done

uninstall:
	rm -rf $(DESTDIR)$(PREFIX)/bin/gpe-filemanager $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-filemanager.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/filemanager

clean:
	rm -f gpe-filemanager $(OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
