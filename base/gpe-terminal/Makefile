PREFIX=/usr/local

install:
	install -m 644 -D gpe-terminal.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-terminal.png
	install -m 644 -D gpe-terminal.desktop $(DESTDIR)$(PREFIX)/share/applications/gpe-terminal.desktop

ipkg:
	rm -rf familiar/dist
	install -D familiar/control familiar/dist/CONTROL/control
	install -D familiar/postinst familiar/dist/CONTROL/postinst
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr install
	chown -R root.root familiar/dist
	ipkg-build familiar/dist
