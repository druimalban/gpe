# Makefile for PDA Maze

# by Bill Kendrick
# bill@newbreedsoftware.com
# http://www.newbreedsoftware.com/

# May 23, 2001 - September 4, 2001

PREFIX = /usr/local

CC=$(PROGPREFIX)gcc
STRIP=$(PROGPREFIX)strip

CFLAGS=-Os -fomit-frame-pointer -Wall -I/usr/include $(EXTRA_CFLAGS) `gdk-pixbuf-config --cflags` -DPREFIX=\"$(PREFIX)\"
XLIB=-L$(LIBDIR) -lX11 `gdk-pixbuf-config --libs`


all:	host

install-program: all pdamaze.desktop
	install -d $(DESTDIR)$(PREFIX)/bin
	install pdamaze.host $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)$(PREFIX)/share/pdamaze
	for i in images/*.png; do install -m 664 $$i $(DESTDIR)$(PREFIX)/share/pdamaze/; done
	install -d $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 pdamaze.desktop $(DESTDIR)$(PREFIX)/share/applications/
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 pdamaze.png $(DESTDIR)$(PREFIX)/share/pixmaps

host:
	make \
		TARGET=pdamaze.host pdamaze.host \
		PROGPREFIX= \
		LIBDIR=/usr/X11R6/lib

mips-svr4:
	make \
		TARGET=pdamaze.mips pdamaze.mips \
		PROGPREFIX=mipsel-linux- \
		LIBDIR=/usr/mipsel-linux/lib/X11

mips-snow:
	make \
		TARGET=pdamaze.snow pdamaze.snow \
		PROGPREFIX=mipsel-linux- \
		EXTRA_CFLAGS="-Os -B/opt/snow-gcc/lib/snow/" \
		LIBDIR=/opt/snow-gcc/lib/snow


clean:
	-rm pdamaze.host pdamaze.mips pdamaze.snow


$(TARGET):	pdamaze.c
	$(CC) $(CFLAGS) pdamaze.c $(XLIB) -o $(TARGET)
	$(STRIP) $(TARGET)
	$(STRIP) --remove-section .compact_rel $(TARGET)
	$(STRIP) --remove-section .note $(TARGET)
	$(STRIP) --remove-section .comment $(TARGET)

include ../../base/build/Makefile.dpkg_ipkg
include ../../base/build/Makefile.translation

