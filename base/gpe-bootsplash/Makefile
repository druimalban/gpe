PACKAGE = gpe-bootsplash
VERSION = 1.9

CFLAGS = `pkg-config --cflags gdk-pixbuf-2.0 librsvg-2.0` `pkg-config --libs gdk-pixbuf-2.0 librsvg-2.0` -DGTK2
SPLASH = splash2.png

all: gpe-bootsplash

gpe-bootsplash: splash.c
	$(CC) $(CFLAGS) -fomit-frame-pointer -Os splash.c -o gpe-bootsplash

install-program: gpe-bootsplash
	mkdir -p $(DESTDIR)/usr/bin
	mkdir -p $(DESTDIR)/usr/share/gpe
	mkdir -p $(DESTDIR)/etc/rcS.d
	mkdir -p $(DESTDIR)/etc/init.d
	cp gpe-bootsplash.init $(DESTDIR)/etc/init.d/gpe-bootsplash
	#install -m 644 $(SPLASH) $(DESTDIR)/usr/share/gpe/splash.png
	install -s gpe-bootsplash $(DESTDIR)/usr/bin/
	ln -sf ../init.d/gpe-bootsplash $(DESTDIR)/etc/rcS.d/S00bootsplash

clean:
	rm -f gpe-bootsplash splash.o

include ../build/Makefile.dpkg_ipkg
