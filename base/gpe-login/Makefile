gpe-login: gpe-login.o
	$(CC) -o $@ $< `gtk-config --libs` -lcrypt -lgdk_imlib

gpe-login.o: gpe-login.c
	$(CC) -c -o $@ $< `gtk-config --cflags` -Os -Wall -fomit-frame-pointer

install:
	cp gpe-login $(DESTDIR)/usr/bin/
	strip $(DESTDIR)/usr/bin/gpe-login

ipkg:
	make DESTDIR=`pwd`/dist install
	ipkg-build dist

clean:
	rm -f gpe-login gpe-login.o
