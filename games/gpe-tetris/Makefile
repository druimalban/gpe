PREFIX = /usr/local
PROGRAM = gpe-tetris
OBJS = misc.o highscore.o tetris.o interface.o
BIN_PATH = $(PREFIX)/bin
HIGHSCORE_PATH = $(PREFIX)/share/gpe-tetris
GROUP = games
HIGHSCORE_FILE = $(HIGHSCORE_PATH)/highscore.dat
DEBUG = yes
CVSBUILD = yes

CC = gcc

CFLAGS = `gdk-pixbuf-config --cflags` `gtk-config --cflags` -DHIGHSCORE_FILE=\"$(HIGHSCORE_FILE)\" -DDOWN_DROPS -D_GNU_SOURCE -Wall
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
	$(CC) $(DEFINES) $(CFLAGS) $(OBJS) -o $(PROGRAM) $(LIBS) $(LDFLAGS)

clean: 
	rm -f *.o $(PROGRAM)

install:
	strip $(PROGRAM)
	install -d $(BIN_PATH)
	install -d $(HIGHSCORE_PATH)
	install -d $(PREFIX)/share/gpe/pixmaps/default/tetris
	install -D pixmaps/stop.png $(PREFIX)/share/gpe/pixmaps/default/tetris/stop.png
	install -D pixmaps/pause.png $(PREFIX)/share/gpe/pixmaps/default/tetris/pause.png
	install -D pixmaps/highscores.png $(PREFIX)/share/gpe/pixmaps/default/tetris/highscores.png
	install -D $(PROGRAM) $(BIN_PATH)/$(PROGRAM)

uninstall:
	rm -i $(BIN_PATH)/$(PROGRAM)
	rm -i $(HIGHSCORE_FILE)
	rmdir $(HIGHSCORE_PATH)

dist:
	-cd .. ; tar c gtktetris-0.5 | gzip -9 > gtktetris-0.5.tar.gz
