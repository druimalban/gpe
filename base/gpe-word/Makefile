PREFIX = /usr/local
PACKAGE = gpe-word
DEBUG = no
CVSBUILD = yes
GTK2 = yes

MEMBERS = main

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

CFLAGS += -I../libdm
LDFLAGS += -L../libdm -ldm

CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: gpe-word

gpe-word: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install: install-program

install-program: all
	install -D gpe-word $(DESTDIR)$(PREFIX)/bin/gpe-word
	install -D -m 644 gpe-word.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-word.png
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE)
	strip $(DESTDIR)$(PREFIX)/bin/gpe-word

clean:
	rm -f gpe-word $(OBJS)


