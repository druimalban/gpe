PREFIX = /usr/local
PACKAGE = gpe-filemanager
DEBUG = no
CVSBUILD = yes
VERSION = 0.12
MIME_PIXMAPS = `cd pixmaps/document-icons && ls *.png`
SPINNER_PIXMAPS = `cd pixmaps/spinner && ls *.png`

MEMBERS = main mime-sql mime-programs-sql bluetooth

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

CFLAGS += $(GPECFLAGS) `pkg-config --cflags gnome-vfs-2.0 dbus-glib-1`
CPPFLAGS += -DDBUS_API_SUBJECT_TO_CHANGE

CFLAGS += -D_GNU_SOURCE -Wall

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif

LDFLAGS += $(GPELIBS) -lsqlite `pkg-config --libs gnome-vfs-2.0 dbus-glib-1`

CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: gpe-filemanager

gpe-filemanager: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install-program: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -s gpe-filemanager $(DESTDIR)$(PREFIX)/bin/gpe-filemanager
	install -D -m 644 gpe-filemanager.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-filemanager.png
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/
	for i in $(MIME_PIXMAPS); do install -D -m 644 pixmaps/document-icons/$$i $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/filemanager/document-icons/$$i; done
	#for i in $(SPINNER_PIXMAPS); do install -D -m 644 pixmaps/spinner/$$i $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/filemanager/spinner/$$i; done
	install -m 644 pixmaps/zoom_in.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/filemanager/zoom_in.png
	install -m 644 pixmaps/zoom_out.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/filemanager/zoom_out.png

uninstall:
	rm -rf $(DESTDIR)$(PREFIX)/bin/gpe-filemanager $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-filemanager.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/filemanager

clean:
	rm -f gpe-filemanager $(OBJS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
