PREFIX=/usr/local

install:
	install -D gpe-terminal.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-terminal.png
	install -D gpe-terminal.menu $(DESTDIR)$(PREFIX)/lib/menu/gpe-terminal

ipkg:
	rm -rf familiar/dist
	install -D familiar/control familiar/dist/CONTROL/control
	install -D familiar/postinst familiar/dist/CONTROL/postinst
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr install
	chown -R root.root familiar/dist
	ipkg-build familiar/dist
