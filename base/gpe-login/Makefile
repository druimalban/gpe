PREFIX = /usr/local
PACKAGE = gpe-login

CFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags` -Os -Wall -fomit-frame-pointer -I../libgpewidget
CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

all: $(PACKAGE) mo

mo:
	make -C po

install-po:
	make -C po install DESTDIR=$(DESTDIR) PREFIX=$(PREFIX)

all: gpe-login gpe-dm

gpe-login: gpe-login.o
	$(CC) -o $@ $< -L../libgpewidget -lgpewidget `gtk-config --libs` -lcrypt

gpe-dm: gpe-dm.o
	$(CC) -o $@ $<

install: install-po
	install -D $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -D $(PACKAGE)-setup $(DESTDIR)/etc/X11/$(PACKAGE)-setup
	strip $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)

ipkg:
	rm -rf dist
	mkdir -p dist/CONTROL
	cp control dist/CONTROL/control
	make DESTDIR=`pwd`/dist PREFIX=/usr install
	chown -R root.root dist
	ipkg-build dist

clean:
	rm -f $(PACKAGE) gpe-login.o
