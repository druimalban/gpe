PREFIX = /usr/local
PACKAGE = gpe-login

CFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags` -Os -Wall -fomit-frame-pointer -I../libgpewidget
CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

all: $(PACKAGE) mo

mo:
	make -C po

install-po:
	make -C po install DESTDIR=$(DESTDIR) PREFIX=$(PREFIX)

update-po: po/$(PACKAGE).pot

po/$(PACKAGE).pot: $(SOURCES)
	xgettext -k_ -o po/$(PACKAGE).pot $^

all: gpe-login gpe-dm

gpe-login: gpe-login.o
	$(CC) -o $@ $< -L../libgpewidget -lgpewidget `gtk-config --libs` -lcrypt

gpe-dm: gpe-dm.o
	$(CC) -o $@ $<

install: install-dm install-login

install-login: install-po
	install -D gpe-login $(DESTDIR)$(PREFIX)/bin/gpe-login
	install -D gpe-login-setup $(DESTDIR)/etc/X11/gpe-login-setup
	strip $(DESTDIR)$(PREFIX)/bin/gpe-login

install-dm:
	install -D gpe-dm $(DESTDIR)$(PREFIX)/bin/gpe-dm
	strip $(DESTDIR)$(PREFIX)/bin/gpe-dm

install-scripts:
	install -D -m 644 X11/Xsession $(DESTDIR)/etc/X11/Xsession
	install -D -m 644 X11/Xinit $(DESTDIR)/etc/X11/Xinit
	install -D -m 644 X11/Xserver $(DESTDIR)/etc/X11/Xserver
	for i in X11/Xsession-gpe.d X11/Xinit.d; do mkdir -p $(DESTDIR)/etc/$$i; FILES=`echo $$i/* | sed "s:$$i/CVS::"`; cp $$FILES $(DESTDIR)/etc/$$i/; done

ipkg:
	rm -rf familiar/dist.gpe-login
	mkdir -p familiar/dist.gpe-login/CONTROL
	cp familiar/control.gpe-login familiar/dist.gpe-login/CONTROL/control
	make DESTDIR=`pwd`/familiar/dist.gpe-login PREFIX=/usr install-login
	chown -R root.root familiar/dist.gpe-login
	ipkg-build familiar/dist.gpe-login
	#
	rm -rf familiar/dist.gpe-dm
	mkdir -p familiar/dist.gpe-dm/CONTROL
	cp familiar/control.gpe-dm familiar/dist.gpe-dm/CONTROL/control
	make DESTDIR=`pwd`/familiar/dist.gpe-dm PREFIX=/usr install-dm
	chown -R root.root familiar/dist.gpe-dm
	ipkg-build familiar/dist.gpe-dm
	#
	rm -rf familiar/dist.gpe-session-scripts
	mkdir -p familiar/dist.gpe-session-scripts/CONTROL
	cp familiar/control.gpe-session-scripts familiar/dist.gpe-session-scripts/CONTROL/control
	make DESTDIR=`pwd`/familiar/dist.gpe-session-scripts PREFIX=/usr install-scripts
	chown -R root.root familiar/dist.gpe-session-scripts
	ipkg-build familiar/dist.gpe-session-scripts

clean:
	rm -f $(PACKAGE) gpe-login.o
