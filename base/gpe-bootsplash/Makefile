PACKAGE = gpe-bootsplash
VERSION = 1.10
CVSBUILD = yes

PACKAGE_CFLAGS = `pkg-config --cflags gdk-pixbuf-2.0 librsvg-2.0` `pkg-config --libs gdk-pixbuf-2.0 librsvg-2.0` -DGTK2
CFLAGS = -fomit-frame-pointer -Os

SPLASH = splash2.png

all: gpe-bootsplash

gpe-bootsplash: splash.c
	$(CC) $(CFLAGS) $(PACKAGE_CFLAGS) $(LDFLAGS) splash.c -o gpe-bootsplash

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

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

include $(BUILD)/Makefile.dpkg_ipkg

