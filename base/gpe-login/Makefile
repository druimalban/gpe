PREFIX = /usr/local

CFLAGS = `gtk-config --cflags` -Os -Wall -fomit-frame-pointer -I../libgpewidget
CFLAGS += -DPACKAGE=\"gpe-login\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

all: gpe-login mo

mo:
	make -C po

install-po:
	make -C po install DESTDIR=$(DESTDIR) PREFIX=$(PREFIX)

gpe-login: gpe-login.o
	$(CC) -o $@ $< -lgpewidget `gtk-config --libs` -lcrypt -lgdk_imlib -L../libgpewidget

gpe-login.o: gpe-login.c
	$(CC) -c $(CFLAGS) -o $@ $<

install: install-po
	install -D gpe-login $(DESTDIR)$(PREFIX)/bin/gpe-login
	install -D gpe-login-setup $(DESTDIR)/etc/X11/gpe-login-setup
	strip $(DESTDIR)$(PREFIX)/bin/gpe-login

ipkg:
	rm -rf dist
	mkdir -p dist/CONTROL
	cp control dist/CONTROL/control
	make DESTDIR=`pwd`/dist PREFIX=/usr install
	chown -R root.root dist
	ipkg-build dist

clean:
	rm -f gpe-login gpe-login.o
