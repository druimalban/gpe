PREFIX = /usr/local
PACKAGE = gpe-irc
DEBUG = yes
CVSBUILD = yes
GTK2 = yes

MEMBERS = main irc irc_input dictionary
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

all: gpe-irc

gpe-irc: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install: install-program

install-program: all
	install -D gpe-irc $(DESTDIR)$(PREFIX)/bin/gpe-irc
	install -D -m 644 gpe-irc.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-irc.png
	install -d $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/irc
	install -d $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/irc/smileys
	install -m 644 pixmaps/globe.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/irc/globe.png
	install -m 644 pixmaps/quote.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/irc/quote.png
	install -m 644 pixmaps/smileys/happy.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/irc/smileys/happy.png
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE)
	strip $(DESTDIR)$(PREFIX)/bin/gpe-irc

clean:
	rm -f gpe-irc $(OBJS)