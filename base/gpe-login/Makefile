gpe-login: gpe-login.o
	$(CC) -o $@ $< `gtk-config --libs` -lcrypt -lgdk_imlib

gpe-login.o: gpe-login.c
	$(CC) -c -o $@ $< `gtk-config --cflags` -Os -Wall -fomit-frame-pointer

install:
	install -D gpe-login $(DESTDIR)/usr/bin/gpe-login
	install -D gpe-login-setup $(DESTDIR)/etc/X11
	strip $(DESTDIR)/usr/bin/gpe-login

ipkg:
	rm -rf dist
	mkdir -p dist/CONTROL
	cp control dist/CONTROL/control
	make DESTDIR=`pwd`/dist install
	chown -R root.root dist
	ipkg-build dist

clean:
	rm -f gpe-login gpe-login.o
