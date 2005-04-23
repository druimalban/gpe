PREFIX = /usr/local
PACKAGE = gpe-tetris
OBJS = misc.o highscore.o tetris.o interface.o
BIN_PATH = $(DESTDIR)$(PREFIX)/bin
HIGHSCORE_PATH = $(PREFIX)/share/gpe-tetris
GROUP = games
HIGHSCORE_FILE = $(HIGHSCORE_PATH)/highscore.dat
DEBUG = yes
CVSBUILD = yes
VERSION = 0.6-4


GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

PACKAGE_CFLAGS = $(GTKCFLAGS) -DHIGHSCORE_FILE=\"$(HIGHSCORE_FILE)\" -DDOWN_DROPS -D_GNU_SOURCE -Wall -DPREFIX=\"$(PREFIX)\" -DPACKAGE=\"$(PACKAGE)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
PACKAGE_LDFLAGS = $(GTKLDFLAGS)

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS += -g
else
CFLAGS += -Os -fomit-frame-pointer
endif

ifeq ($(CVSBUILD),yes)
PACKAGE_CFLAGS += -I../../base/libgpewidget
PACKAGE_LDFLAGS += -L../../base/libgpewidget
BUILD = ../../base/build
else
PACKAGE_CFLAGS += $(GPECFLAGS)
BUILD = build
endif
PACKAGE_LDFLAGS += $(GPELIBS)

all: $(OBJS)
	$(CC) $(DEFINES) $(CFLAGS) $(OBJS) $(LDFLAGS) $(PACKAGE_LDFLAGS) -o $(PACKAGE)

clean: 
	rm -f *.o $(PACKAGE)

install-program: all
	install -d $(BIN_PATH)
	install -d $(DESTDIR)$(HIGHSCORE_PATH)
	install -d $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/tetris
	install -m 644 pixmaps/stop.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/tetris/stop.png
	install -m 644 pixmaps/pause.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/tetris/pause.png
	install -m 644 pixmaps/highscores.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/tetris/highscores.png
	install -s $(PACKAGE) $(BIN_PATH)/$(PACKAGE)
	install -d $(DESTDIR)$(PREFIX)/share/applications
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 gpe-tetris.desktop $(DESTDIR)$(PREFIX)/share/applications/
	install -m 644 gpe-tetris.png $(DESTDIR)$(PREFIX)/share/pixmaps/

uninstall:
	rm -i $(BIN_PATH)/$(PACKAGE)
	rm -i $(HIGHSCORE_FILE)
	rmdir $(HIGHSCORE_PATH)

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation

