PREFIX = /usr/local
PROGRAM = gpe-tetris
OBJS = misc.o highscore.o tetris.o interface.o
BIN_PATH = $(DESTDIR)$(PREFIX)/bin
HIGHSCORE_PATH = $(PREFIX)/share/gpe-tetris
GROUP = games
HIGHSCORE_FILE = $(HIGHSCORE_PATH)/highscore.dat
DEBUG = yes
CVSBUILD = yes
VERSION = 0.6-1
GTK2 = yes

CC = gcc

ifeq ($(GTK2),yes)
GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`
CONTROL = control1
else
GTKCFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags`
GTKLDFLAGS = `gtk-config --libs`
CONTROL = control
endif

CFLAGS = $(GTKCFLAGS) -DHIGHSCORE_FILE=\"$(HIGHSCORE_FILE)\" -DDOWN_DROPS -D_GNU_SOURCE -Wall -DPREFIX=\"$(PREFIX)\" -DPACKAGE=\"$(PROGRAM)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
LDFLAGS = $(GTKLDFLAGS)

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS += -g #-lefence
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

all: $(OBJS)
	$(CC) $(DEFINES) $(CFLAGS) $(OBJS) -o $(PROGRAM) $(LDFLAGS)

clean: 
	rm -f *.o $(PROGRAM)

install-program: all
	install -d $(BIN_PATH)
	install -d $(DESTDIR)$(HIGHSCORE_PATH)
	install -d $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/tetris
	install -m 644 pixmaps/stop.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/tetris/stop.png
	install -m 644 pixmaps/pause.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/tetris/pause.png
	install -m 644 pixmaps/highscores.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/tetris/highscores.png
	install -s $(PROGRAM) $(BIN_PATH)/$(PROGRAM)
	install -d $(DESTDIR)$(PREFIX)/share/applications
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 gpe-tetris.desktop $(DESTDIR)$(PREFIX)/share/applications/
	install -m 644 gpe-tetris.png $(DESTDIR)$(PREFIX)/share/pixmaps/

uninstall:
	rm -i $(BIN_PATH)/$(PROGRAM)
	rm -i $(HIGHSCORE_FILE)
	rmdir $(HIGHSCORE_PATH)

dist:
	-cd .. ; tar c gtktetris-0.5 | gzip -9 > gtktetris-0.5.tar.gz

include ../../base/build/Makefile.dpkg_ipkg
-include $(DEPS)
