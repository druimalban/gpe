#set up the symlinks for the package
GTK2 = yes

ifeq ($(GTK2),yes)
CONTROL = control1
CFLAGS = `pkg-config --cflags gdk-pixbuf-2.0` `pkg-config --libs gdk-pixbuf-2.0` -DGTK2
SPLASH = splash2.png
else
CONTROL = control
CFLAGS = `gdk-pixbuf-config --cflags` `gdk-pixbuf-config --libs`
SPLASH = splash.png
endif

all: gpe-bootsplash

gpe-bootsplash: splash.c
	gcc $(CFLAGS) -fomit-frame-pointer -Os splash.c -o gpe-bootsplash
	strip gpe-bootsplash

ipkg:
	rm -rf dist
	mkdir -p dist/CONTROL
	mkdir -p dist/usr/bin
	mkdir -p dist/usr/share/gpe
	mkdir -p dist/etc/rcS.d
	mkdir -p dist/etc/init.d
	cp gpe-bootsplash.init dist/etc/init.d/gpe-bootsplash
	install -m 644 $(SPLASH) dist/usr/share/gpe/splash.png
	cp gpe-bootsplash dist/usr/bin/
	cp $(CONTROL) dist/CONTROL/control
	ln -s ../init.d/gpe-bootsplash dist/etc/rcS.d/S11bootsplash
	ipkg-build dist
