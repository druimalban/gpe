all: bluez-pin

bluez-pin: bluez-pin.o
	$(CC) -o $@ $< `gtk-config --libs` -lsqlite -lgdk_imlib -L../libgpewidget -lgpewidget

bluez-pin.o: bluez-pin.c
	$(CC) -c -o $@ $< `gtk-config --cflags` -I../libgpewidget -Os -Wall -fomit-frame-pointer

install:
	install -D bluez-pin $(DESTDIR)/usr/bin/bluez-pin
	install -D bt-logo.png $(DESTDIR)/usr/share/pixmaps/bt-logo.png
	strip $(DESTDIR)/usr/bin/bluez-pin

clean:
	rm bluez-pin bluez-pin.o

ipkg:	
	rm -rf dist
	mkdir -p dist/CONTROL
	cp control dist/CONTROL/control
	make DESTDIR=`pwd`/dist install
	chown -R root.root dist
	ipkg-build dist
