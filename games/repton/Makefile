#
# Repton for Linux - Makefile
#
# $Id$
#

CC=		gcc
COPT=		-O2
CFLAGS=		-I. -Wall $(COPT) `sdl-config --cflags` -MD
LDFLAGS=
LIBS=		`sdl-config --libs`

PREFIX = /usr/local
BUILD = ../../base/build
VERSION = 1.3

BINDIR=		$(DESTDIR)$(PREFIX)/bin
LIBDIR=		$(DESTDIR)$(PREFIX)/lib
BINOWN=		root
BINGRP=		root

MAPS=		maps/baby.rep maps/future.rep maps/orient.rep \
		maps/school.rep maps/work.rep maps/africa.rep \
		maps/babyj.rep maps/now.rep maps/pirate.rep \
		maps/teens.rep maps/america.rep maps/egypt.rep \
		maps/oap.rep maps/prehist.rep maps/toccata.rep \
		maps/arctic.rep maps/finale.rep maps/oceans.rep \
		maps/prelude.rep maps/victori.rep

BMPS=		bmps/main.bmp bmps/credits.bmp bmps/endepis.bmp \
		bmps/endlevel.bmp bmps/passwd.bmp bmps/start.bmp

SNDS=		snds/bridge1.ub snds/chirp.ub snds/choo.ub \
		snds/click.ub snds/cp70_eb2.ub snds/doo_eb3.ub \
		snds/dx_ep_a2.ub snds/music_box.ub snds/orchhit.ub \
		snds/pinknois.ub snds/screech.ub snds/space.ub

MEMBERS=	main kernel fileio sdl showmap sound \
		err soundIt

.SUFFIXES: .d

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

PROG=		repton

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

install-program: $(PROG) $(MAPS) $(BMPS)
	install -m 755 -d -o $(BINOWN) -g $(BINGRP) $(BINDIR)
	install -m 755 -d -o $(BINOWN) -g $(BINGRP) $(LIBDIR)/repton/maps
	install -m 755 -d -o $(BINOWN) -g $(BINGRP) $(LIBDIR)/repton/bmps
	install -m 755 -d -o $(BINOWN) -g $(BINGRP) $(LIBDIR)/repton/snds
	install -s -m 555 -o $(BINOWN) -g $(BINGRP) $(PROG) $(BINDIR)
	install -m 444 -o $(BINOWN) -g $(BINGRP) $(MAPS) $(LIBDIR)/repton/maps
	install -m 444 -o $(BINOWN) -g $(BINGRP) $(BMPS) $(LIBDIR)/repton/bmps
	install -m 444 -o $(BINOWN) -g $(BINGRP) $(SNDS) $(LIBDIR)/repton/snds

uninstall:
	rm -f $(BINDIR)/$(PROG)
	rm -rf $(LIBDIR)/repton

clean:
	rm -f *~ *.o $(PROG)

# Dependencies
err.o: err.c
fileio.o: fileio.c repton.h
kernel.o: kernel.c repton.h
main.o: main.c repton.h pathnames.h version.h
mapsprites.o: mapsprites.c
mouse.o: mouse.c mouse.h mouse_arrow.c
mouse_arrow.o: mouse_arrow.c
palette.o: palette.c
showmap.o: showmap.c repton.h mapsprites.c
sound.o: sound.c repton.h pathnames.h soundIt.h
soundIt.o: soundIt.c soundIt.h
svgalib.o: svgalib.c repton.h mouse.h pathnames.h palette.c

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
-include $(DEPS)
