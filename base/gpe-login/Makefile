PREFIX = /usr/local
PACKAGE = gpe-login
CVSBUILD = yes

CFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags` -Os -Wall -fomit-frame-pointer
CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libgpewidget
LDFLAGS += -L../libgpewidget
else
CFLAGS += -I/usr/include/gpe
endif
LDFLAGS += -lgpewidget

all: gpe-login gpe-dm gpe-lock-display mo

mo:
	make -C po

install-po: mo
	make -C po install DESTDIR=$(DESTDIR) PREFIX=$(PREFIX)

update-po: po/$(PACKAGE).pot

po/$(PACKAGE).pot: $(SOURCES)
	xgettext -k_ -o po/$(PACKAGE).pot $^

gpe-login: gpe-login.o
	$(CC) -o $@ $< $(LDFLAGS) `gtk-config --libs` -lcrypt

gpe-dm: gpe-dm.o
	$(CC) -o $@ $< -L/usr/X11R6/lib -lX11

gpe-lock-display: gpe-lock-display.o
	$(CC) -o $@ $< -L/usr/X11R6/lib -lX11

install: install-dm install-login

install-login: gpe-login install-po
	install -D gpe-login $(DESTDIR)$(PREFIX)/bin/gpe-login
	install -D gpe-lock-display $(DESTDIR)$(PREFIX)/bin/gpe-lock-display
	install -D gpe-login.setup $(DESTDIR)/etc/X11/gpe-login.setup
	install -D gpe-login.keylaunchrc $(DESTDIR)/etc/X11/gpe-login.keylaunchrc
	install -D -m 644 gpe-login.buttons $(DESTDIR)/etc/X11/gpe-login.buttons
	install -D gpe-login.suspend $(DESTDIR)/etc/suspend-scripts/S98lock-display
	install -D gpe-login.session $(DESTDIR)/etc/X11/Xsession-gpe.d/50autolock
	strip $(DESTDIR)$(PREFIX)/bin/gpe-login
	strip $(DESTDIR)$(PREFIX)/bin/gpe-lock-display

install-dm: gpe-dm
	install -D gpe-dm $(DESTDIR)$(PREFIX)/bin/gpe-dm
	strip $(DESTDIR)$(PREFIX)/bin/gpe-dm
	mkdir -p $(DESTDIR)/etc/init.d
	mkdir -p $(DESTDIR)/etc/rc2.d
	install gpe-dm.init $(DESTDIR)/etc/init.d/gpe-dm
	ln -sf /etc/init.d/gpe-dm $(DESTDIR)/etc/rc2.d/S99gpe-dm

install-scripts:
	install -D X11/Xsession $(DESTDIR)/etc/X11/Xsession
	install -D X11/Xinit $(DESTDIR)/etc/X11/Xinit
	install -D X11/Xserver $(DESTDIR)/etc/X11/Xserver
	install -D X11/gpe-login.pre-session $(DESTDIR)/etc/X11/gpe-login.pre-session
	for i in X11/Xsession-gpe.d X11/Xinit.d; do mkdir -p $(DESTDIR)/etc/$$i; FILES=`echo $$i/* | sed "s:$$i/CVS::"`; cp $$FILES $(DESTDIR)/etc/$$i/; done

ipkg:
	make PREFIX=/usr clean all
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
	mkdir -p familiar/dist.gpe-session-scripts/usr/lib/menu
	cp familiar/gpe-logout.menu familiar/dist.gpe-session-scripts/usr/lib/menu/gpe-logout
	#
	chown -R root.root familiar/dist.gpe-session-scripts
	ipkg-build familiar/dist.gpe-session-scripts

clean:
	rm -f gpe-login gpe-login.o gpe-dm gpe-dm.o gpe-lock-display gpe-lock-display.o
