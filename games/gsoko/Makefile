
PACKAGE_CFLAGS = -Wall $(GPECFLAGS) $(GTKCFLAGS) -DPREFIX=\"$(PREFIX)\"
OBJS = control.o game.o gfx.o gsoko.o init.o interface.o move.o
VERSION = 0.4.2-gpe5
PACKAGE = gsoko

PREFIX = /usr/local
BUILD = ../../base/build

all:	gsoko

gsoko:	$(OBJS)
	$(CC) $(CFLAGS) $(PACKAGE_CFLAGS) -o $@ $(OBJS) `pkg-config --libs gtk+-2.0`

clean:
	rm -f *.o
	rm -f gsoko

#.c.o:
#	$(CC) $(CFLAGS) $(PACKAGE_CFLAGS) -c `pkg-config --cflags gtk+-2.0` $< -DPREFIX=\"$(PREFIX)\"

LEVELS = 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50
IMGS = box.png box2.png man.png tile.png tile2.png wall.png

install-program: gsoko gsoko.desktop
	install -d $(DESTDIR)$(PREFIX)/bin
	install -s gsoko $(DESTDIR)$(PREFIX)/bin/gsoko
	install -d $(DESTDIR)$(PREFIX)/share/gsoko/img
	install -d $(DESTDIR)$(PREFIX)/share/gsoko/levels
	for i in $(LEVELS); do install -m 644 levels/$$i.lev $(DESTDIR)$(PREFIX)/share/gsoko/levels/$$i.lev; done
	for i in $(IMGS); do install -m 644 img/$$i $(DESTDIR)$(PREFIX)/share/gsoko/img/$$i; done
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 gsoko.png $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -d $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 gsoko.desktop $(DESTDIR)$(PREFIX)/share/applications/

control.o:	control.c gsoko.h
game.o:		game.c gsoko.h
gfx.o:		gfx.c gsoko.h
gsoko.o:	gsoko.c gsoko.h
init.o:		init.c gsoko.h
interface.o:	interface.c gsoko.h
move.o:		move.c gsoko.h

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation

