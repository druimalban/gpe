PREFIX = /usr/local
PACKAGE = ipaq-sleep

all: ipaq-sleep

ipaq-sleep: ipaq-sleep.c
	$(CC) -Os -fomit-frame-pointer -o ipaq-sleep ipaq-sleep.c -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 -lXext -lXss -Wall

install: all
	install -D $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	strip $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)/etc/init.d
	mkdir -p $(DESTDIR)/etc/rc2.d
	install ipaq-sleep.init $(DESTDIR)/etc/init.d/ipaq-sleep
	ln -sf /etc/init.d/ipaq-sleep $(DESTDIR)/etc/rc2.d/S99ipaq-sleep
	install ipaq-sleep.conf $(DESTDIR)/etc/ipaq-sleep.conf
	install -D ipaq-sleep.session $(DESTDIR)/etc/X11/Xsession.d/69ipaq-sleep

upstream: clean
	( VERSION=`dpkg-parsechangelog | grep ^Version | cut -d\  -f2 | cut -d- -f1`; cd ..; cp -a $(PACKAGE) $(PACKAGE)-$$VERSION.orig; tar czf $(PACKAGE)_$$VERSION.orig.tar.gz --exclude=$(PACKAGE)-$$VERSION.orig/debian --exclude=$(PACKAGE)-$$VERSION.orig/CVS --exclude=$(PACKAGE)-$$VERSION.orig/familiar $(PACKAGE)-$$VERSION.orig ; rm -rf $(PACKAGE)-$$VERSION.orig )

dpkg: upstream
	( VERSION=`dpkg-parsechangelog | grep ^Version | cut -d\  -f2 | cut -d- -f1`; TMP=/var/tmp/debian/$(PACKAGE); PV=$(PACKAGE)-$$VERSION; rm -rf $$TMP; mkdir -p $$TMP; cp ../$(PACKAGE)_$$VERSION.orig.tar.gz $$TMP; (cd $$TMP; tar xzf $(PACKAGE)_$$VERSION.orig.tar.gz ; mv $$PV.orig $$PV ); mkdir -p $$TMP/$$PV/debian; cp debian/* $$TMP/$$PV/debian; cd $$TMP/$$PV; fakeroot dpkg-buildpackage  ; cd ..; rm -rf $$PV)

ipkg:
	rm -rf familiar/dist
	mkdir -p familiar/dist/CONTROL
	cp familiar/control familiar/dist/CONTROL
	cp familiar/conffiles familiar/dist/CONTROL
	cp familiar/preinst familiar/dist/CONTROL
	cp familiar/postinst familiar/dist/CONTROL
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr install
	chown -R root.root familiar/dist
	ipkg-build familiar/dist

clean:
	rm -f $(PACKAGE) $(OBJS)
