PREFIX = /usr/local
PACKAGE = gpe-edit
DEBUG = no
CVSBUILD = yes
GTK2 = yes

MEMBERS = main
LINGUAS = de

ifeq ($(GTK2),yes)
CFLAGS = `pkg-config --cflags gtk+-2.0`
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
LDFLAGS += -lgpewidget

ifeq ($(GTK2),yes)
LDFLAGS += `pkg-config --libs gtk+-2.0`
else
LDFLAGS += `gtk-config --libs` `gdk-pixbuf-config --libs`
endif

CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: gpe-edit all-mo

gpe-edit: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install: install-program install-mo

install-program: all
	install -D gpe-edit $(DESTDIR)$(PREFIX)/bin/gpe-edit
	install -D -m 644 gpe-edit.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-edit.png
	install -D -m 644 gpe-edit.menu $(DESTDIR)$(PREFIX)/lib/menu/gpe-edit
	install -D -m 644 gpe-edit.mime $(DESTDIR)/etc/mime-handlers/gpe-edit.mime
	strip $(DESTDIR)$(PREFIX)/bin/gpe-edit

clean:
	rm -f gpe-edit $(OBJS)

include ../build/Makefile.translation
include ../build/Makefile.dpkg_ipkg

