all: bluez-pin

bluez-pin: bluez-pin.o
	$(CC) -o $@ $< -L../libgpewidget -lgpewidget `gtk-config --libs` -lsqlite

bluez-pin.o: bluez-pin.c
	$(CC) -c -o $@ $< `gtk-config --cflags` `gdk-pixbuf-config --cflags` -I../libgpewidget -Os -Wall -fomit-frame-pointer

install: bluez-pin
	install -D bluez-pin $(DESTDIR)/usr/bin/bluez-pin
	install -D bt-logo.png $(DESTDIR)/usr/share/pixmaps/bt-logo.png
	strip $(DESTDIR)/usr/bin/bluez-pin

clean:
	rm -f bluez-pin bluez-pin.o

ipkg:	
	rm -rf dist
	mkdir -p dist/CONTROL
	cp control dist/CONTROL/control
	make DESTDIR=`pwd`/dist install
	chown -R root.root dist
	ipkg-build dist
