#set up the symlinks for the package
all: gpe-bootsplash

gpe-bootsplash: splash.c
	gcc `gdk-pixbuf-config --cflags` -Os splash.c `gdk-pixbuf-config --libs` -o gpe-bootsplash
	strip gpe-bootsplash

ipkg:
	rm -rf dist
	mkdir -p dist/CONTROL
	mkdir -p dist/usr/bin
	mkdir -p dist/usr/share/gpe
	mkdir -p dist/etc/rcS.d
	mkdir -p dist/etc/init.d
	cp gpe-bootsplash.init dist/etc/init.d/gpe-bootsplash
	cp splash.png dist/usr/share/gpe/
	cp gpe-bootsplash dist/usr/bin/
	cp control dist/CONTROL
	ln -s ../init.d/gpe-bootsplash dist/etc/rcS.d/S06bootsplash
	ipkg-build dist
