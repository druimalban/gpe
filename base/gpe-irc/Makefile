PREFIX = /usr/local
PACKAGE = gpe-irc
VERSION = 0.02
DEBUG = yes
CVSBUILD = yes

MEMBERS = main irc irc_input dictionary general_config general_config_sql networks_config networks_config_sql irc_parse ctcp
LINGUAS = 

CFLAGS = `pkg-config --cflags gtk+-2.0`
CONTROL = control1

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
BUILD = ../build
else
CFLAGS += -I/usr/include/gpe
BUILD = build
endif
LDFLAGS += -lgpewidget -lsqlite

LDFLAGS += `pkg-config --libs gtk+-2.0`

CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install-program: all
	install -D $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -D -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	install -d $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/irc
	install -d $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/irc/smileys
	install -d $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/irc/colors
	install -m 644 pixmaps/globe.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/irc/globe.png
	install -m 644 pixmaps/quote.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/irc/quote.png
	install -m 644 pixmaps/smileys/happy.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/irc/smileys/happy.png
	install -m 644 pixmaps/colors/black.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/irc/colors/black.png
	install -m 644 pixmaps/colors/white.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/irc/colors/white.png
	install -m 644 pixmaps/colors/red.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/irc/colors/red.png
	install -m 644 pixmaps/colors/green.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/irc/colors/green.png
	install -m 644 pixmaps/colors/blue.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/irc/colors/blue.png
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE)
	strip $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)

clean:
	rm -f $(PACKAGE) $(OBJS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
