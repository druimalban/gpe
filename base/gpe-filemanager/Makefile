PREFIX = /usr/local
PACKAGE = gpe-filemanager
DEBUG = yes
CVSBUILD = yes
MIME_PIXMAPS = `cd pixmaps/document-icons && ls *.png`
SPINNER_PIXMAPS = `cd pixmaps/spinner && ls *.png`
MIME_HANDLERS = `cd mime-handlers && ls *.mime`
GTK2 = yes

MEMBERS = main mime-sql mime-programs-sql
MEMBERS2 = update_mime_db hash mime-sql mime-programs-sql

ifeq ($(GTK2),yes)
CFLAGS = `pkg-config --cflags gtk+-2.0 gnome-vfs-2.0` -DGTK_ENABLE_BROKEN
CONTROL = control1
else
CFLAGS = `gdk-pixbuf-config --cflags` `gtk-config --cflags` 
CONTROL = control
endif

CFLAGS += -D_GNU_SOURCE -Wall
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libgpewidget
LDFLAGS += -L../libgpewidget
else
CFLAGS += -I/usr/include/gpe
endif
LDFLAGS += -lgpewidget -lsqlite

ifeq ($(GTK2),yes)
LDFLAGS += `pkg-config --libs gtk+-2.0 gnome-vfs-2.0`
else
LDFLAGS += `gtk-config --libs` `gdk-pixbuf-config --libs`
endif

CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.o,$(MEMBERS))
OBJS2 = $(patsubst %,%.o,$(MEMBERS2))
SOURCES = $(patsubst %,%.c,$(MEMBERS))
SOURCES2 = $(patsubst %,%.c,$(MEMBERS2))

all: gpe-filemanager update_mime_db

gpe-filemanager: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

update_mime_db: $(OBJS2)
	$(CC) -o $@ $^ $(LDFLAGS)

install: all
	install -D gpe-filemanager $(DESTDIR)$(PREFIX)/bin/gpe-filemanager
	install -D update_mime_db $(DESTDIR)$(PREFIX)/bin/update_mime_db
	install -D -m 644 gpe-filemanager.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-filemanager.png
	install -D -m 644 gpe-filemanager.menu $(DESTDIR)$(PREFIX)/lib/menu/gpe-filemanager
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE)
	for i in $(MIME_HANDLERS); do install -D mime-handlers/$$i $(DESTDIR)/etc/mime-handlers/$$i; done
	for i in $(MIME_PIXMAPS); do install -D pixmaps/document-icons/$$i $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/filemanager/document-icons/$$i; done
	for i in $(SPINNER_PIXMAPS); do install -D pixmaps/spinner/$$i $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/filemanager/spinner/$$i; done
	install -m 644 pixmaps/zoom_in.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/filemanager/zoom_in.png
	install -m 644 pixmaps/zoom_out.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/filemanager/zoom_out.png
	strip $(DESTDIR)$(PREFIX)/bin/gpe-filemanager
	strip $(DESTDIR)$(PREFIX)/bin/update_mime_db

uninstall:
	rm -rf $(DESTDIR)$(PREFIX)/bin/gpe-filemanager $(DESTDIR)$(PREFIX)/bin/update_mime_db $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-filemanager.png $(DESTDIR)$(PREFIX)/lib/menu/gpe-filemanager $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/filemanager ~/.gpe/mime_types

clean:
	rm -f gpe-filemanager $(OBJS)

include ../build/Makefile.dpkg_ipkg
include ../build/Makefile.translation
