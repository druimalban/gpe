all: bluez-pin

bluez-pin: bluez-pin.o
	$(CC) -o $@ $< `gtk-config --libs` -lsqlite -lgdk_imlib

bluez-pin.o: bluez-pin.c
	$(CC) -c -o $@ $< `gtk-config --cflags` -Os -Wall -fomit-frame-pointer

install:
	install -D bluez-pin $(DESTDIR)/usr/bin/bluez-pin
	install -D bt-logo.png $(DESTDIR)/usr/share/pixmaps/bt-logo.png
	strip $(DESTDIR)/usr/bin/bluez-pin

clean:
	rm bluez-pin bluez-pin.o

ipkg:
	make DESTDIR=`pwd`/dist install
	ipkg-build dist
