PREFIX = /usr/local
PROGRAM = gpe-tetris
OBJS = misc.o highscore.o tetris.o interface.o
BIN_PATH = $(DESTDIR)$(PREFIX)/bin
HIGHSCORE_PATH = $(PREFIX)/share/gpe-tetris
GROUP = games
HIGHSCORE_FILE = $(HIGHSCORE_PATH)/highscore.dat
DEBUG = yes
CVSBUILD = yes

CC = gcc

CFLAGS = `gdk-pixbuf-config --cflags` `gtk-config --cflags` -DHIGHSCORE_FILE=\"$(HIGHSCORE_FILE)\" -DDOWN_DROPS -D_GNU_SOURCE -Wall -DPREFIX=\"$(PREFIX)\" -DPACKAGE=\"$(PROGRAM)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../../base/libgpewidget
LDFLAGS += -L../../base/libgpewidget
else
CFLAGS += -I/usr/include/gpe
endif
LDFLAGS += -lgpewidget

LIBS = `gdk-pixbuf-config --libs` `gtk-config --libs`

all: $(OBJS)
	$(CC) $(DEFINES) $(CFLAGS) $(OBJS) -o $(PROGRAM) $(LDFLAGS) $(LIBS)

clean: 
	rm -f *.o $(PROGRAM)

install: all
	strip $(PROGRAM)
	install -d $(BIN_PATH)
	install -d $(DESTDIR)$(HIGHSCORE_PATH)
	install -d $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/tetris
	install -m 644 pixmaps/stop.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/tetris/stop.png
	install -m 644 pixmaps/pause.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/tetris/pause.png
	install -m 644 pixmaps/highscores.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/tetris/highscores.png
	install $(PROGRAM) $(BIN_PATH)/$(PROGRAM)
	install -d $(DESTDIR)$(PREFIX)/lib/menu
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 gpe-tetris.menu $(DESTDIR)$(PREFIX)/lib/menu/gpe-tetris
	install -m 644 gpe-tetris.png $(DESTDIR)$(PREFIX)/share/pixmaps/

uninstall:
	rm -i $(BIN_PATH)/$(PROGRAM)
	rm -i $(HIGHSCORE_FILE)
	rmdir $(HIGHSCORE_PATH)

dist:
	-cd .. ; tar c gtktetris-0.5 | gzip -9 > gtktetris-0.5.tar.gz

ipkg:
	rm -rf familiar/dist
	mkdir -p familiar/dist/CONTROL
	cp familiar/control familiar/postinst familiar/dist/CONTROL
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr DEBUG=no install
	chown -R root.root familiar/dist
	ipkg-build familiar/dist
